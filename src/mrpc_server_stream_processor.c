#include "private/mrpc_common.h"

#include "private/mrpc_server_stream_processor.h"
#include "private/mrpc_server_request_processor.h"
#include "private/mrpc_interface.h"
#include "ff/ff_event.h"
#include "ff/ff_pool.h"
#include "ff/ff_blocking_queue.h"
#include "ff/ff_stream.h"
#include "ff/ff_core.h"

/**
 * the maximum number of mrpc_server_request_processor instances, which can be simultaneously invoked
 * by the current mrpc_server_stream_processor.
 * This number should be less or equal to the 0x100, because of restriction on the size (1 bytes) of request_id
 * field in the mrpc client-server protocol. request_id is assigned for each currently running request
 * and is used for association with the corresponding asynchronous response.
 * So there are 0x100 maximum request processors can be invoked in parallel.
 */
#define MAX_REQUEST_PROCESSORS_CNT 0x100

/**
 * the maximum number of mrpc_packet packets, which can be used by the instance of the
 * mrpc_server_stream_processor. These packets are used when receiving data from the underlying stream.
 * Also they are used by the mrpc_server_request_processor when serializing rpc responses.
 * In order to avoid deadlocks this number must be not less than (2 * MAX_REQUEST_PROCESSORS_CNT).
 */
#define MAX_PACKETS_CNT (2 * MAX_REQUEST_PROCESSORS_CNT)

enum server_stream_processor_state
{
	STATE_WORKING,
	STATE_STOP_INITIATED,
	STATE_STOPPED,
};

struct mrpc_server_stream_processor
{
	mrpc_server_stream_processor_release_func release_func;
	void *release_func_ctx;
	mrpc_server_stream_processor_release_id_func release_id_func;
	void *release_id_func_ctx;
	struct ff_event *writer_stop_event;
	struct ff_event *request_processors_stop_event;
	struct ff_pool *request_processors_pool;
	struct ff_pool *packets_pool;
	struct ff_blocking_queue *writer_queue;
	struct mrpc_server_request_processor **active_request_processors;
	int id;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_stream *stream;
	int active_request_processors_cnt;
	enum server_stream_processor_state state;
};

static struct mrpc_packet *acquire_server_packet(struct mrpc_server_stream_processor *stream_processor)
{
	struct mrpc_packet *packet;

	ff_assert(stream_processor->state != STATE_STOPPED);
	packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
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

static struct mrpc_server_request_processor *acquire_request_processor(struct mrpc_server_stream_processor *stream_processor, uint8_t request_id)
{
	struct mrpc_server_request_processor *request_processor;

	ff_assert(stream_processor->active_request_processors_cnt >= 0);
	ff_assert(stream_processor->active_request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	ff_assert(stream_processor->state != STATE_STOPPED);

	request_processor = (struct mrpc_server_request_processor *) ff_pool_acquire_entry(stream_processor->request_processors_pool);
	ff_assert(stream_processor->active_request_processors[request_id] == NULL);
	stream_processor->active_request_processors[request_id] = request_processor;
	stream_processor->active_request_processors_cnt++;
	ff_assert(stream_processor->active_request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	if (stream_processor->active_request_processors_cnt == 1)
	{
		ff_event_reset(stream_processor->request_processors_stop_event);
	}
	return request_processor;
}

static void release_request_processor(void *ctx, struct mrpc_server_request_processor *request_processor, uint8_t request_id)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);

	ff_assert(stream_processor->active_request_processors_cnt > 0);
	ff_assert(stream_processor->active_request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	ff_assert(stream_processor->active_request_processors[request_id] == request_processor);

	ff_pool_release_entry(stream_processor->request_processors_pool, request_processor);
	stream_processor->active_request_processors[request_id] = NULL;
	stream_processor->active_request_processors_cnt--;
	if (stream_processor->active_request_processors_cnt == 0)
	{
		ff_event_set(stream_processor->request_processors_stop_event);
	}
}

static void notify_request_processor_error(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);
	mrpc_server_stream_processor_stop_async(stream_processor);
}

static void *create_request_processor(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct mrpc_server_request_processor *request_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);
	request_processor = mrpc_server_request_processor_create(release_request_processor, stream_processor, notify_request_processor_error, stream_processor,
		acquire_packet, release_packet, stream_processor, stream_processor->writer_queue);
	return request_processor;
}

static void delete_request_processor(void *ctx)
{
	struct mrpc_server_request_processor *request_processor;

	request_processor = (struct mrpc_server_request_processor *) ctx;
	mrpc_server_request_processor_delete(request_processor);
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

static void stop_all_request_processors(struct mrpc_server_stream_processor *stream_processor)
{
	struct mrpc_server_request_processor **active_request_processors;
	int i;

	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	active_request_processors = stream_processor->active_request_processors;
	for (i = 0; i < MAX_REQUEST_PROCESSORS_CNT; i++)
	{
		struct mrpc_server_request_processor *request_processor;

		request_processor = active_request_processors[i];
		if (request_processor != NULL)
		{
			mrpc_server_request_processor_stop_async(request_processor);
		}
	}
	ff_event_wait(stream_processor->request_processors_stop_event);
	ff_assert(stream_processor->active_request_processors_cnt == 0);
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
	struct mrpc_server_request_processor **active_request_processors;
	struct mrpc_interface *service_interface;
	void *service_ctx;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;

	ff_assert(stream_processor->active_request_processors_cnt == 0);
	ff_assert(stream_processor->service_interface != NULL);
	ff_assert(stream_processor->stream != NULL);
	ff_assert(stream_processor->state != STATE_STOPPED);

	start_stream_writer(stream_processor);
	ff_event_set(stream_processor->request_processors_stop_event);
	stream = stream_processor->stream;
	active_request_processors = stream_processor->active_request_processors;
	service_interface = stream_processor->service_interface;
	service_ctx = stream_processor->service_ctx;
	for (;;)
	{
		struct mrpc_packet *packet;
		struct mrpc_server_request_processor *request_processor;
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
		request_processor = active_request_processors[request_id];
		if (packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_SINGLE)
		{
			if (request_processor != NULL)
			{
				ff_log_debug(L"there is the request_processor with the given request_id=%lu, but the packet received "
							 L"from the stream=%p indicates that this request_processor shouldn't exist. stream_processor=%p, packet_type=%d",
							 	(uint32_t) request_id, stream, stream_processor, (int) packet_type);
				release_server_packet(stream_processor, packet);
				break;
			}
			request_processor = acquire_request_processor(stream_processor, request_id);
			mrpc_server_request_processor_start(request_processor, service_interface, service_ctx, request_id);
		}
		else
		{
			/* packet_type is MRPC_PACKET_MIDDLE or MRPC_PACKET_LAST */
			if (request_processor == NULL)
			{
				ff_log_debug(L"there is no request_processor with the given request_id=%lu, but the packet received "
							 L"from the stream=%p indicates that this request_processor should exist. stream_processor=%p, packet_type=%d",
							 	(uint32_t) request_id, stream, stream_processor, (int) packet_type);
				release_server_packet(stream_processor, packet);
				break;
			}
		}
		mrpc_server_request_processor_push_packet(request_processor, packet);
	}
	mrpc_server_stream_processor_stop_async(stream_processor);
	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	stop_all_request_processors(stream_processor);
	stop_stream_writer(stream_processor);
	ff_stream_delete(stream_processor->stream);
	stream_processor->service_interface = NULL;
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
	stream_processor->request_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_processors_pool = ff_pool_create(MAX_REQUEST_PROCESSORS_CNT, create_request_processor, stream_processor, delete_request_processor);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);

	/* in fact, writer_queue's size must be unlimited in order to avoid blocking of
	 * mrpc_server_request_processor's worker on mrpc_packet_stream's flush,
	 * which is the last operation before the client will receive response and will be able
	 * to send new request with the same request_id to the server. If the worker
	 * will block on the flush, then the server can receive new request with the request_id,
	 * which belongs to worker, before the corresponding mrpc_server_request_processor will be released,
	 * so it won't be able to process the new request, which will lead to client-server connection termination.
	 *
	 * However, the stream processor can operate only with packets from the packets_pool with
	 * limited size (MAX_PACKETS_CNT). So, it is safe to set writer_queue's size to MAX_PACKETS_CNT,
	 * because this limit won't never be overflowed.
	 */
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_PACKETS_CNT);
	stream_processor->active_request_processors = (struct mrpc_server_request_processor **) ff_calloc(MAX_REQUEST_PROCESSORS_CNT, sizeof(stream_processor->active_request_processors[0]));
	stream_processor->id = id;

	stream_processor->service_interface = NULL;
	stream_processor->service_ctx = NULL;
	stream_processor->stream = NULL;
	stream_processor->active_request_processors_cnt = 0;
	stream_processor->state = STATE_STOPPED;

	return stream_processor;
}

void mrpc_server_stream_processor_delete(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->service_interface == NULL);
	ff_assert(stream_processor->service_ctx == NULL);
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->active_request_processors_cnt == 0);
	ff_assert(stream_processor->state == STATE_STOPPED);

	stream_processor->release_id_func(stream_processor->release_id_func_ctx, stream_processor->id);
	ff_free(stream_processor->active_request_processors);
	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_pool_delete(stream_processor->packets_pool);
	ff_pool_delete(stream_processor->request_processors_pool);
	ff_event_delete(stream_processor->request_processors_stop_event);
	ff_event_delete(stream_processor->writer_stop_event);
	ff_free(stream_processor);
}

void mrpc_server_stream_processor_start(struct mrpc_server_stream_processor *stream_processor,
	struct mrpc_interface *service_interface, void *service_ctx, struct ff_stream *stream)
{
	ff_assert(stream_processor->service_interface == NULL);
	ff_assert(stream_processor->service_ctx == NULL);
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->state == STATE_STOPPED);

	ff_assert(service_interface != NULL);
	ff_assert(stream != NULL);

	stream_processor->state = STATE_WORKING;
	stream_processor->service_interface = service_interface;
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

