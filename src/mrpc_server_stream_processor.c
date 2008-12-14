#include "private/mrpc_common.h"

#include "private/mrpc_server_stream_processor.h"
#include "private/mrpc_packet_stream.h"
#include "private/mrpc_server_stream_handler.h"
#include "ff/ff_event.h"
#include "ff/ff_pool.h"
#include "ff/ff_blocking_queue.h"
#include "ff/ff_stream.h"
#include "ff/ff_core.h"

/**
 * the maximum number of request streams, which can be simultaneously created by the mrpc_client_stream_processor.
 * This number should be less or equal to the 0x100, because of restriction on the size (1 bytes) of request_id
 * field in the mrpc client-server protocol. request_id is assigned for each currently running request
 * and is used for association with the corresponding asynchronous response.
 * So there are 0x100 maximum request streams can be created in parallel.
 */
#define MAX_REQUEST_STREAMS_CNT 0x100

/**
 * the maximum number of mrpc_packet packets, which can be used by the instance of the
 * mrpc_server_stream_processor. These packets are used when receiving data from the underlying stream.
 * Also they are used by the request streams when serializing rpc responses.
 * In order to avoid deadlocks this number must be not less than (2 * MAX_REQUEST_STREAMS_CNT).
 */
#define MAX_PACKETS_CNT (2 * MAX_REQUEST_STREAMS_CNT)

enum server_stream_processor_state
{
	STATE_WORKING,
	STATE_STOP_INITIATED,
	STATE_STOPPED,
};

struct request_stream
{
	struct mrpc_server_stream_processor *stream_processor;
	struct mrpc_packet_stream *packet_stream;
	struct ff_stream *stream;
	uint8_t request_id;
};

struct mrpc_server_stream_processor
{
	mrpc_server_stream_processor_release_func release_func;
	void *release_func_ctx;
	mrpc_server_stream_processor_release_id_func release_id_func;
	void *release_id_func_ctx;
	struct ff_event *writer_stop_event;
	struct ff_event *request_streams_stop_event;
	struct ff_pool *request_streams_pool;
	struct ff_pool *packets_pool;
	struct ff_blocking_queue *writer_queue;
	struct request_stream **active_request_streams;
	mrpc_server_stream_handler stream_handler;
	void *service_ctx;
	struct ff_stream *stream;
	int id;
	int active_request_streams_cnt;
	enum server_stream_processor_state state;
};

static struct mrpc_packet *acquire_server_packet(struct mrpc_server_stream_processor *stream_processor)
{
	struct mrpc_packet *packet;

	ff_assert(stream_processor->state != STATE_STOPPED);
	ff_pool_acquire_entry(stream_processor->packets_pool, (void **) &packet);
	return packet;
}

static void release_server_packet(struct mrpc_server_stream_processor *stream_processor, struct mrpc_packet *packet)
{
	ff_assert(stream_processor->state != STATE_STOPPED);
	mrpc_packet_reset(packet);
	ff_pool_release_entry(stream_processor->packets_pool, packet);
}

static struct mrpc_packet *acquire_packet(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct mrpc_packet *packet;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	packet = acquire_server_packet(stream_processor);
	return packet;
}

static void release_packet(void *ctx, struct mrpc_packet *packet)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	release_server_packet(stream_processor, packet);
}

static void skip_writer_queue_packets(struct mrpc_server_stream_processor *stream_processor)
{
	struct ff_blocking_queue *writer_queue;

	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	writer_queue = stream_processor->writer_queue;
	for (;;)
	{
		struct mrpc_packet *packet;

		ff_blocking_queue_get(writer_queue, (const void **) &packet);
		if (packet == NULL)
		{
			int is_empty;

			is_empty = ff_blocking_queue_is_empty(writer_queue);
			ff_assert(is_empty);
			break;
		}
		release_server_packet(stream_processor, packet);
	}
}

static void stream_writer_func(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct ff_blocking_queue *writer_queue;
	struct ff_stream *stream;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	ff_assert(stream_processor->stream != NULL);
	ff_assert(stream_processor->state != STATE_STOPPED);
	writer_queue = stream_processor->writer_queue;
	stream = stream_processor->stream;
	for (;;)
	{
		struct mrpc_packet *packet;
		int is_empty;
		enum ff_result result;

		ff_blocking_queue_get(writer_queue, (const void **) &packet);
		if (packet == NULL)
		{
			ff_assert(stream_processor->state == STATE_STOP_INITIATED);
			is_empty = ff_blocking_queue_is_empty(writer_queue);
			ff_assert(is_empty);
			break;
		}
		result = mrpc_packet_write_to_stream(packet, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot write packet=%p to the stream=%p of the stream_processor=%p. See previous messages for more info",
				packet, stream, stream_processor);
		}
		release_server_packet(stream_processor, packet);

		/* below is an optimization, which is used for minimizing the number of
		 * usually expensive ff_stream_flush() calls. These calls are invoked only
		 * if the writer_queue is empty at the moment. If we won't flush the stream
		 * this moment moment, then potential deadlock can occur:
		 * 1) server serializes rpc response into the mrpc_packets and pushes them into the writer_queue.
		 * 2) this function writes these packets into the stream.
		 *    Usually this means that the packets' content is buffered in the underlying stream buffer
		 *    and not sent to the remote side (i.e. client).
		 * 3) this function blocks awaiting for the new packets in the writer_packet.
		 * 4) deadlock:
		 *     - client is blocked awaiting for response from the server, which is still buffered on the server side;
		 *     - server is blocked awaiting for next request from the client.
		 */
		is_empty = ff_blocking_queue_is_empty(writer_queue);
		if (result == FF_SUCCESS && is_empty)
		{
			result = ff_stream_flush(stream);
			if (result != FF_SUCCESS)
			{
				ff_log_debug(L"cannot flush the stream=%p of the stream_processor=%p. See previous messages for more info", stream_processor->stream, stream_processor);
			}
		}
		if (result == FF_FAILURE)
		{
			mrpc_server_stream_processor_stop_async(stream_processor);
			ff_assert(stream_processor->state == STATE_STOP_INITIATED);
			skip_writer_queue_packets(stream_processor);
			break;
		}
	}
	ff_event_set(stream_processor->writer_stop_event);
}

static void delete_request_stream_wrapper(void *ctx)
{
	/* nothing to do */
}

static enum ff_result read_from_request_stream_wrapper(void *ctx, void *buf, int len)
{
	struct request_stream *request_stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ctx;
	ff_assert(request_stream->packet_stream != NULL);
	result = mrpc_packet_stream_read(request_stream->packet_stream, buf, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot read data from the request_stream=%p to the buf=%p, len=%d. See previous messages for more info", request_stream, buf, len);
	}
	return result;
}

static enum ff_result write_to_request_stream_wrapper(void *ctx, const void *buf, int len)
{
	struct request_stream *request_stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ctx;
	ff_assert(request_stream->packet_stream != NULL);
	result = mrpc_packet_stream_write(request_stream->packet_stream, buf, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write data to the request_stream=%p from the buf=%p, len=%d. See previous messages for more info", request_stream, buf, len);
	}
	return result;
}

static enum ff_result flush_request_stream_wrapper(void *ctx)
{
	struct request_stream *request_stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ctx;
	ff_assert(request_stream->packet_stream != NULL);
	result = mrpc_packet_stream_flush(request_stream->packet_stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot flush the request_stream=%p. See previous messages for more info", request_stream);
	}
	return result;
}

static void disconnect_request_stream_wrapper(void *ctx)
{
	struct request_stream *request_stream;

	request_stream = (struct request_stream *) ctx;
	ff_assert(request_stream->packet_stream != NULL);
	mrpc_packet_stream_disconnect(request_stream->packet_stream);
}

static const struct ff_stream_vtable request_stream_wrapper_vtable =
{
	delete_request_stream_wrapper,
	read_from_request_stream_wrapper,
	write_to_request_stream_wrapper,
	flush_request_stream_wrapper,
	disconnect_request_stream_wrapper
};

static struct ff_stream *create_request_stream_wrapper(struct request_stream *request_stream)
{
	struct ff_stream *stream;

	ff_assert(request_stream->packet_stream != NULL);
	stream = ff_stream_create(&request_stream_wrapper_vtable, request_stream);
	return stream;
}

static struct request_stream *acquire_request_stream(struct mrpc_server_stream_processor *stream_processor, uint8_t request_id)
{
	struct request_stream *request_stream;

	ff_assert(stream_processor->active_request_streams_cnt >= 0);
	ff_assert(stream_processor->active_request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);
	ff_assert(stream_processor->state != STATE_STOPPED);

	ff_pool_acquire_entry(stream_processor->request_streams_pool, (void **) &request_stream);
	ff_assert(stream_processor->active_request_streams[request_id] == NULL);
	mrpc_packet_stream_initialize(request_stream->packet_stream, request_id);
	stream_processor->active_request_streams[request_id] = request_stream;

	stream_processor->active_request_streams_cnt++;
	ff_assert(stream_processor->active_request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);
	if (stream_processor->active_request_streams_cnt == 1)
	{
		ff_event_reset(stream_processor->request_streams_stop_event);
	}
	request_stream->request_id = request_id;
	return request_stream;
}

static void release_request_stream(struct mrpc_server_stream_processor *stream_processor, struct request_stream *request_stream)
{
	uint8_t request_id;

	ff_assert(stream_processor->state != STATE_STOPPED);
	ff_assert(stream_processor->active_request_streams_cnt > 0);
	ff_assert(stream_processor->active_request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);

	request_id = request_stream->request_id;
	ff_assert(stream_processor->active_request_streams[request_id] == request_stream);
	mrpc_packet_stream_shutdown(request_stream->packet_stream);
	stream_processor->active_request_streams[request_id] = NULL;
	ff_pool_release_entry(stream_processor->request_streams_pool, request_stream);

	stream_processor->active_request_streams_cnt--;
	if (stream_processor->active_request_streams_cnt == 0)
	{
		ff_event_set(stream_processor->request_streams_stop_event);
	}
}

static void *create_request_stream(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct request_stream *request_stream;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);

	/* The current stream processor can simultaneously use up to MAX_PACKETS_CNT packets.
	 * So, it will be safe to set the reader queue's size to MAX_PACKETS_CNT.
	 */
	request_stream = (struct request_stream *) ff_malloc(sizeof(*request_stream));
	request_stream->stream_processor = stream_processor;
	request_stream->packet_stream = mrpc_packet_stream_create(stream_processor->writer_queue, MAX_PACKETS_CNT, acquire_packet, release_packet, stream_processor);
	request_stream->stream = create_request_stream_wrapper(request_stream);
	request_stream->request_id = 0;

	return request_stream;
}

static void delete_request_stream(void *ctx)
{
	struct request_stream *request_stream;

	request_stream = (struct request_stream *) ctx;
	ff_stream_delete(request_stream->stream);
	mrpc_packet_stream_delete(request_stream->packet_stream);
	ff_free(request_stream);
}

static void process_request_func(void *ctx)
{
	struct request_stream *request_stream;
	struct mrpc_server_stream_processor *stream_processor;
	mrpc_server_stream_handler stream_handler;
	void *service_ctx;
	struct ff_stream *stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ctx;
	ff_assert(request_stream->stream_processor != NULL);
	ff_assert(request_stream->packet_stream != NULL);
	ff_assert(request_stream->stream != NULL);

	stream_processor = request_stream->stream_processor;
	ff_assert(stream_processor->stream_handler != NULL);
	stream_handler = stream_processor->stream_handler;
	service_ctx = stream_processor->service_ctx;
	stream = request_stream->stream;

	result = stream_handler(stream, service_ctx);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot process remote call from the stream=%p using the stream_handler=%p, service_ctx=%p", stream, stream_handler, service_ctx);
		mrpc_server_stream_processor_stop_async(stream_processor);
	}
	release_request_stream(stream_processor, request_stream);
}

static void *create_packet(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct mrpc_packet *packet;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);

	packet = mrpc_packet_create();
	return packet;
}

static void delete_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = (struct mrpc_packet *) ctx;
	mrpc_packet_delete(packet);
}

static void stop_all_request_streams(struct mrpc_server_stream_processor *stream_processor)
{
	struct request_stream **active_request_streams;
	int i;

	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	active_request_streams = stream_processor->active_request_streams;
	for (i = 0; i < MAX_REQUEST_STREAMS_CNT; i++)
	{
		struct request_stream *request_stream;

		request_stream = active_request_streams[i];
		if (request_stream != NULL)
		{
			mrpc_packet_stream_disconnect(request_stream->packet_stream);
		}
	}
	ff_event_wait(stream_processor->request_streams_stop_event);
	ff_assert(stream_processor->active_request_streams_cnt == 0);
}

static void start_stream_writer(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->state != STATE_STOPPED);
	ff_core_fiberpool_execute_async(stream_writer_func, stream_processor);
}

static void stop_stream_writer(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	ff_blocking_queue_put(stream_processor->writer_queue, NULL);
	ff_event_wait(stream_processor->writer_stop_event);
}

static void stream_reader_func(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct ff_stream *stream;
	struct request_stream **active_request_streams;
	mrpc_server_stream_handler stream_handler;
	void *service_ctx;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;

	ff_assert(stream_processor->active_request_streams_cnt == 0);
	ff_assert(stream_processor->stream_handler != NULL);
	ff_assert(stream_processor->stream != NULL);
	ff_assert(stream_processor->state != STATE_STOPPED);

	start_stream_writer(stream_processor);
	ff_event_set(stream_processor->request_streams_stop_event);
	stream = stream_processor->stream;
	active_request_streams = stream_processor->active_request_streams;
	stream_handler = stream_processor->stream_handler;
	service_ctx = stream_processor->service_ctx;
	for (;;)
	{
		struct mrpc_packet *packet;
		struct request_stream *request_stream;
		uint8_t request_id;
		enum mrpc_packet_type packet_type;
		enum ff_result result;

		packet = acquire_server_packet(stream_processor);
		result = mrpc_packet_read_from_stream(packet, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot read the packet=%p from the stream=%p. See previous messages for more info", packet, stream);
			release_server_packet(stream_processor, packet);
			break;
		}

		packet_type = mrpc_packet_get_type(packet);
		request_id = mrpc_packet_get_request_id(packet);
		request_stream = active_request_streams[request_id];
		if (packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_SINGLE)
		{
			if (request_stream != NULL)
			{
				ff_log_debug(L"there is the request_stream with the given request_id=%lu, but the packet received "
							 L"from the stream=%p indicates that this request_stream shouldn't exist. stream_processor=%p, packet_type=%d",
							 	(uint32_t) request_id, stream, stream_processor, (int) packet_type);
				release_server_packet(stream_processor, packet);
				break;
			}
			request_stream = acquire_request_stream(stream_processor, request_id);
			ff_core_fiberpool_execute_async(process_request_func, request_stream);
		}
		else
		{
			/* packet_type is MRPC_PACKET_MIDDLE or MRPC_PACKET_LAST */
			if (request_stream == NULL)
			{
				ff_log_debug(L"there is no request_stream with the given request_id=%lu, but the packet received "
							 L"from the stream=%p indicates that this request_stream should exist. stream_processor=%p, packet_type=%d",
							 	(uint32_t) request_id, stream, stream_processor, (int) packet_type);
				release_server_packet(stream_processor, packet);
				break;
			}
		}
		mrpc_packet_stream_push_packet(request_stream->packet_stream, packet);
	}
	mrpc_server_stream_processor_stop_async(stream_processor);
	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	stop_all_request_streams(stream_processor);
	stop_stream_writer(stream_processor);
	ff_stream_delete(stream_processor->stream);
	stream_processor->stream_handler = NULL;
	stream_processor->service_ctx = NULL;
	stream_processor->stream = NULL;
	stream_processor->state = STATE_STOPPED;
	stream_processor->release_func(stream_processor->release_func_ctx, stream_processor);
}

struct mrpc_server_stream_processor *mrpc_server_stream_processor_create(mrpc_server_stream_processor_release_func release_func, void *release_func_ctx,
	mrpc_server_stream_processor_release_id_func release_id_func, void *release_id_func_ctx, int id)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ff_malloc(sizeof(*stream_processor));
	stream_processor->release_func = release_func;
	stream_processor->release_func_ctx = release_func_ctx;
	stream_processor->release_id_func = release_id_func;
	stream_processor->release_id_func_ctx = release_id_func_ctx;
	stream_processor->writer_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_streams_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_streams_pool = ff_pool_create(MAX_REQUEST_STREAMS_CNT, create_request_stream, stream_processor, delete_request_stream);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);

	/* in fact, writer_queue's size must be unlimited in order to avoid blocking of process_request_func on packet_stream flush,
	 * which is the last operation before the client will receive response and will be able
	 * to send new request with the same request_id to the server. If the worker
	 * will block on the flush, then the server can receive new request with the request_id,
	 * which belongs to request_stream, before the corresponding request stream will be released,
	 * so it won't be able to process the new request, which will lead to client-server connection termination.
	 *
	 * However, the stream processor can operate only with packets from the packets_pool with
	 * limited size (MAX_PACKETS_CNT). So, it is safe to set writer_queue's size to MAX_PACKETS_CNT,
	 * because this limit won't never be overflowed.
	 */
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_PACKETS_CNT);
	stream_processor->active_request_streams = (struct request_stream **) ff_calloc(MAX_REQUEST_STREAMS_CNT, sizeof(stream_processor->active_request_streams[0]));
	stream_processor->id = id;

	stream_processor->stream_handler = NULL;
	stream_processor->service_ctx = NULL;
	stream_processor->stream = NULL;
	stream_processor->active_request_streams_cnt = 0;
	stream_processor->state = STATE_STOPPED;

	return stream_processor;
}

void mrpc_server_stream_processor_delete(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream_handler == NULL);
	ff_assert(stream_processor->service_ctx == NULL);
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->active_request_streams_cnt == 0);
	ff_assert(stream_processor->state == STATE_STOPPED);

	stream_processor->release_id_func(stream_processor->release_id_func_ctx, stream_processor->id);
	ff_free(stream_processor->active_request_streams);
	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_pool_delete(stream_processor->packets_pool);
	ff_pool_delete(stream_processor->request_streams_pool);
	ff_event_delete(stream_processor->request_streams_stop_event);
	ff_event_delete(stream_processor->writer_stop_event);
	ff_free(stream_processor);
}

void mrpc_server_stream_processor_start(struct mrpc_server_stream_processor *stream_processor, mrpc_server_stream_handler stream_handler, void *service_ctx, struct ff_stream *stream)
{
	ff_assert(stream_handler != NULL);
	ff_assert(stream != NULL);

	ff_assert(stream_processor->stream_handler == NULL);
	ff_assert(stream_processor->service_ctx == NULL);
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->state == STATE_STOPPED);

	stream_processor->state = STATE_WORKING;
	stream_processor->stream_handler = stream_handler;
	stream_processor->service_ctx = service_ctx;
	stream_processor->stream = stream;
	ff_core_fiberpool_execute_async(stream_reader_func, stream_processor);
}

void mrpc_server_stream_processor_stop_async(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->state != STATE_STOPPED);

	if (stream_processor->state == STATE_WORKING)
	{
		ff_assert(stream_processor->stream != NULL);
		stream_processor->state = STATE_STOP_INITIATED;
		ff_stream_disconnect(stream_processor->stream);
	}
	else
	{
		ff_assert(stream_processor->state == STATE_STOP_INITIATED);
		ff_log_debug(L"the stream_processor=%p didn't yet stopped, so do nothing", stream_processor);
	}
}

int mrpc_server_stream_processor_get_id(struct mrpc_server_stream_processor *stream_processor)
{
	return stream_processor->id;
}
