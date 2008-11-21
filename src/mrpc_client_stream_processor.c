#include "private/mrpc_common.h"

#include "private/mrpc_client_stream_processor.h"
#include "private/mrpc_packet_stream.h"
#include "private/mrpc_bitmap.h"
#include "ff/ff_blocking_queue.h"
#include "ff/ff_pool.h"
#include "ff/ff_event.h"
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
 * mrpc_client_stream_processor. These packets are used when receiving data from the underlying stream.
 * Also they are used by the request streams when serializing rpc requests.
 * In order to avoid deadlocks this number must be not less than (2 * MAX_REQUEST_STREAMS_CNT).
 */
#define MAX_PACKETS_CNT (2 * MAX_REQUEST_STREAMS_CNT)

enum client_stream_processor_state
{
	STATE_WORKING,
	STATE_STOP_INITIATED,
	STATE_STOPPED,
};

struct request_stream
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_packet_stream *packet_stream;
	uint8_t request_id;
};

struct mrpc_client_stream_processor
{
	struct ff_blocking_queue *writer_queue;
	struct ff_event *writer_stop_event;
	struct mrpc_bitmap *request_streams_bitmap;
	struct ff_pool *request_streams_pool;
	struct ff_event *request_streams_stop_event;
	struct ff_pool *packets_pool;
	struct request_stream **active_request_streams;
	struct ff_stream *stream;
	int active_request_streams_cnt;
	enum client_stream_processor_state state;
};

static struct mrpc_packet *acquire_client_packet(struct mrpc_client_stream_processor *stream_processor)
{
	struct mrpc_packet *packet;

	ff_assert(stream_processor->state != STATE_STOPPED);
	packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
	return packet;
}

static void release_client_packet(struct mrpc_client_stream_processor *stream_processor, struct mrpc_packet *packet)
{
	ff_assert(stream_processor->state != STATE_STOPPED);
	mrpc_packet_reset(packet);
	ff_pool_release_entry(stream_processor->packets_pool, packet);
}

static struct mrpc_packet *acquire_packet(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_packet *packet;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	packet = acquire_client_packet(stream_processor);
	return packet;
}

static void release_packet(void *ctx, struct mrpc_packet *packet)
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	release_client_packet(stream_processor, packet);
}

static void skip_writer_queue_packets(struct mrpc_client_stream_processor *stream_processor)
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
		release_client_packet(stream_processor, packet);
	}
}

static void stream_writer_func(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct ff_blocking_queue *writer_queue;
	struct ff_stream *stream;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
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
		release_client_packet(stream_processor, packet);

		/* below is an optimization, which is used for minimizing the number of
		 * usually expensive ff_stream_flush() calls. These calls are invoked only
		 * if the writer_queue is empty at the moment. If we won't flush the stream
		 * this moment moment, then potential deadlock can occur:
		 * 1) client serializes rpc request into the mrpc_packets and pushes them into the writer_queue.
		 * 2) this function writes these packets into the stream.
		 *    Usually this means that the packets' content is buffered in the underlying stream buffer
		 *    and not sent to the remote side (i.e. server).
		 * 3) this function blocks awaiting for the new packets in the writer_packet.
		 * 4) deadlock:
		 *     - server is blocked awaiting for request from the client, which is still buffered on the client side;
		 *     - client is blocked awaiting for response from the server.
		 */
		is_empty = ff_blocking_queue_is_empty(writer_queue);
		if (result == FF_SUCCESS && is_empty)
		{
			result = ff_stream_flush(stream);
			if (result != FF_SUCCESS)
			{
				ff_log_debug(L"cannot flush the stream=%p of the stream_processor=%p. See previous messages for more info", stream, stream_processor);
			}
		}
		if (result != FF_SUCCESS)
		{
			mrpc_client_stream_processor_stop_async(stream_processor);
			ff_assert(stream_processor->state == STATE_STOP_INITIATED);
			skip_writer_queue_packets(stream_processor);
			break;
		}
	}
	ff_event_set(stream_processor->writer_stop_event);
}

static uint8_t acquire_request_id(struct mrpc_client_stream_processor *stream_processor)
{
	int request_id;

	ff_assert(stream_processor->state != STATE_STOPPED);
	request_id = mrpc_bitmap_acquire_bit(stream_processor->request_streams_bitmap);
	ff_assert(request_id >= 0);
	ff_assert(request_id < MAX_REQUEST_STREAMS_CNT);

	return (uint8_t) request_id;
}

static void release_request_id(struct mrpc_client_stream_processor *stream_processor, uint8_t request_id)
{
	mrpc_bitmap_release_bit(stream_processor->request_streams_bitmap, request_id);
}

static void *create_request_stream(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct request_stream *request_stream;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);

	/* The current stream processor can simultaneously use up to MAX_PACKETS_CNT packets.
	 * So, it will be safe to set the reader queue's size to MAX_PACKETS_CNT.
	 */
	request_stream = (struct request_stream *) ff_malloc(sizeof(*request_stream));
	request_stream->stream_processor = stream_processor;
	request_stream->packet_stream = mrpc_packet_stream_create(stream_processor->writer_queue, MAX_PACKETS_CNT, acquire_packet, release_packet, stream_processor);
	request_stream->request_id = acquire_request_id(stream_processor);

	return request_stream;
}

static void delete_request_stream(void *ctx)
{
	struct request_stream *request_stream;

	request_stream = (struct request_stream *) ctx;

	release_request_id(request_stream->stream_processor, request_stream->request_id);
	mrpc_packet_stream_delete(request_stream->packet_stream);
	ff_free(request_stream);
}

static struct request_stream *acquire_request_stream(struct mrpc_client_stream_processor *stream_processor)
{
	struct request_stream *request_stream;
	uint8_t request_id;

	ff_assert(stream_processor->active_request_streams_cnt >= 0);
	ff_assert(stream_processor->active_request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);
	ff_assert(stream_processor->state != STATE_STOPPED);

	request_stream = (struct request_stream *) ff_pool_acquire_entry(stream_processor->request_streams_pool);
	request_id = request_stream->request_id;
	ff_assert(stream_processor->active_request_streams[request_id] == NULL);
	mrpc_packet_stream_initialize(request_stream->packet_stream, request_id);
	stream_processor->active_request_streams[request_id] = request_stream;

	stream_processor->active_request_streams_cnt++;
	ff_assert(stream_processor->active_request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);
	if (stream_processor->active_request_streams_cnt == 1)
	{
		ff_event_reset(stream_processor->request_streams_stop_event);
	}
	return request_stream;
}

static void release_request_stream(struct mrpc_client_stream_processor *stream_processor, struct request_stream *request_stream)
{
	uint8_t request_id;

	ff_assert(stream_processor->active_request_streams_cnt > 0);
	ff_assert(stream_processor->active_request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);
	ff_assert(stream_processor->state != STATE_STOPPED);

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

static void *create_packet(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_packet *packet;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
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

static void stop_all_request_streams(struct mrpc_client_stream_processor *stream_processor)
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

static void start_stream_writer(struct mrpc_client_stream_processor *stream_processor)
{
	ff_assert(stream_processor->state != STATE_STOPPED);
	ff_core_fiberpool_execute_async(stream_writer_func, stream_processor);
}

static void stop_stream_writer(struct mrpc_client_stream_processor *stream_processor)
{
	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	ff_blocking_queue_put(stream_processor->writer_queue, NULL);
	ff_event_wait(stream_processor->writer_stop_event);
}

static void delete_request_stream_wrapper(struct ff_stream *stream)
{
	struct request_stream *request_stream;

	request_stream = (struct request_stream *) ff_stream_get_ctx(stream);
	ff_assert(request_stream->packet_stream != NULL);
	ff_assert(request_stream->stream_processor != NULL);
	release_request_stream(request_stream->stream_processor, request_stream);
}

static enum ff_result read_from_request_stream_wrapper(struct ff_stream *stream, void *buf, int len)
{
	struct request_stream *request_stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ff_stream_get_ctx(stream);
	ff_assert(request_stream->packet_stream != NULL);
	result = mrpc_packet_stream_read(request_stream->packet_stream, buf, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot read data from the stream=%p to the buf=%p, len=%d. See previous messages for more info", stream, buf, len);
	}
	return result;
}

static enum ff_result write_to_request_stream_wrapper(struct ff_stream *stream, const void *buf, int len)
{
	struct request_stream *request_stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ff_stream_get_ctx(stream);
	ff_assert(request_stream->packet_stream != NULL);
	result = mrpc_packet_stream_write(request_stream->packet_stream, buf, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write data to the stream=%p from the buf=%p, len=%d. See previous messages for more info", stream, buf, len);
	}
	return result;
}

static enum ff_result flush_request_stream_wrapper(struct ff_stream *stream)
{
	struct request_stream *request_stream;
	enum ff_result result;

	request_stream = (struct request_stream *) ff_stream_get_ctx(stream);
	ff_assert(request_stream->packet_stream != NULL);
	result = mrpc_packet_stream_flush(request_stream->packet_stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot flush the stream=%p. See previous messages for more info", stream);
	}
	return result;
}

static void disconnect_request_stream_wrapper(struct ff_stream *stream)
{
	struct request_stream *request_stream;

	request_stream = (struct request_stream *) ff_stream_get_ctx(stream);
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

static struct ff_stream *create_request_stream_wrapper(struct mrpc_client_stream_processor *stream_processor)
{
	struct request_stream *request_stream;
	struct ff_stream *stream;

	ff_assert(stream_processor->state == STATE_WORKING);

	request_stream = acquire_request_stream(stream_processor);
	ff_assert(request_stream->packet_stream != NULL);
	ff_assert(request_stream->stream_processor == stream_processor);
	stream = ff_stream_create(&request_stream_wrapper_vtable, request_stream);
	return stream;
}

struct mrpc_client_stream_processor *mrpc_client_stream_processor_create()
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ff_malloc(sizeof(*stream_processor));

	/* the maximum number of packets in the writer_queue is limited by the packets_pool size (MAX_PACKETS_CNT),
	 * because only packets from those pool can be used by the stream_processor.
	 */
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_PACKETS_CNT);
	stream_processor->writer_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_streams_bitmap = mrpc_bitmap_create(MAX_REQUEST_STREAMS_CNT);
	stream_processor->request_streams_pool = ff_pool_create(MAX_REQUEST_STREAMS_CNT, create_request_stream, stream_processor, delete_request_stream);
	stream_processor->request_streams_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);
	stream_processor->active_request_streams = (struct request_stream **) ff_calloc(MAX_REQUEST_STREAMS_CNT, sizeof(stream_processor->active_request_streams[0]));

	stream_processor->stream = NULL;
	stream_processor->active_request_streams_cnt = 0;
	stream_processor->state = STATE_STOPPED;

	return stream_processor;
}

void mrpc_client_stream_processor_delete(struct mrpc_client_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->active_request_streams_cnt == 0);
	/* state can be either STATE_STOPPED either STATE_STOP_INITIATED.
	 * The second case is possible if the mrpc_client_stream_processor_stop_async()
	 * was called when the state was STATE_STOPPED and the mrpc_client_stream_processor_process_stream() wasn't called
	 * after the mrpc_client_stream_processor_stop_async() call.
	 */
	ff_assert(stream_processor->state != STATE_WORKING);

	ff_free(stream_processor->active_request_streams);
	ff_pool_delete(stream_processor->packets_pool);
	ff_event_delete(stream_processor->request_streams_stop_event);
	ff_pool_delete(stream_processor->request_streams_pool);
	mrpc_bitmap_delete(stream_processor->request_streams_bitmap);
	ff_event_delete(stream_processor->writer_stop_event);
	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_free(stream_processor);
}

void mrpc_client_stream_processor_process_stream(struct mrpc_client_stream_processor *stream_processor, struct ff_stream *stream)
{
	struct request_stream **active_request_streams;

	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->active_request_streams_cnt == 0);
	ff_assert(stream_processor->state != STATE_WORKING);

	if (stream_processor->state == STATE_STOP_INITIATED)
	{
		ff_log_debug(L"the stream_processor=%p cannot process the stream=%p, because it is instructed to stop", stream_processor, stream);
		goto end;
	}
	ff_assert(stream_processor->state == STATE_STOPPED);
	stream_processor->state = STATE_WORKING;
	stream_processor->stream = stream;
	start_stream_writer(stream_processor);
	ff_event_set(stream_processor->request_streams_stop_event);
	active_request_streams = stream_processor->active_request_streams;
	for (;;)
	{
		struct mrpc_packet *packet;
		struct request_stream *request_stream;
		uint8_t request_id;
		enum ff_result result;

		packet = acquire_client_packet(stream_processor);
		result = mrpc_packet_read_from_stream(packet, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot read packet=%p from the stream=%p. See previous messages for more info", packet, stream);
			release_client_packet(stream_processor, packet);
			break;
		}

		request_id = mrpc_packet_get_request_id(packet);
		request_stream = active_request_streams[request_id];
		if (request_stream == NULL)
		{
			ff_log_debug(L"there is no active request_stream for the request_id=%lu read from the stream=%p", (uint32_t) request_id, stream);
			release_client_packet(stream_processor, packet);
			break;
		}
		mrpc_packet_stream_push_packet(request_stream->packet_stream, packet);
	}
	mrpc_client_stream_processor_stop_async(stream_processor);
	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	stop_all_request_streams(stream_processor);
	ff_assert(stream_processor->active_request_streams_cnt == 0);
	stop_stream_writer(stream_processor);
	stream_processor->stream = NULL;

end:
	stream_processor->state = STATE_STOPPED;
}

void mrpc_client_stream_processor_stop_async(struct mrpc_client_stream_processor *stream_processor)
{
	if (stream_processor->state == STATE_WORKING)
	{
		ff_assert(stream_processor->stream != NULL);
		stream_processor->state = STATE_STOP_INITIATED;
		ff_stream_disconnect(stream_processor->stream);
	}
	else if (stream_processor->state == STATE_STOPPED)
	{
		/* the mrpc_client_stream_processor_stop_async() was called
		 * before the mrpc_client_stream_processor_process_stream() call.
		 * This means that the mrpc_client_stream_processor_process_stream() call must
		 * return immediately.
		 */
		ff_log_debug(L"the stream_processor=%p is already stopped. Instruct it to stop next time the mrpc_client_stream_processor_process_stream() will be called", stream_processor);
		ff_assert(stream_processor->stream == NULL);
		ff_assert(stream_processor->active_request_streams_cnt == 0);
		stream_processor->state = STATE_STOP_INITIATED;
	}
	else
	{
		ff_assert(stream_processor->state == STATE_STOP_INITIATED);
		ff_log_debug(L"the stream_processor=%p didn't yet stopped, so do nothing", stream_processor);
	}
}

struct ff_stream *mrpc_client_stream_processor_create_request_stream(struct mrpc_client_stream_processor *stream_processor)
{
	struct ff_stream *stream = NULL;

	if (stream_processor->state == STATE_WORKING)
	{
		stream = create_request_stream_wrapper(stream_processor);
	}
	else
	{
		ff_log_debug(L"the stream_processor=%p cannot create request stream, because it is stopped or its stop process is initiated");
	}
	return stream;
}
