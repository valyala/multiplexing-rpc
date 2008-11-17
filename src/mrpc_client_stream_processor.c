#include "private/mrpc_common.h"

#include "private/mrpc_client_stream_processor.h"
#include "private/mrpc_client_request_processor.h"
#include "private/mrpc_bitmap.h"
#include "ff/ff_blocking_queue.h"
#include "ff/ff_pool.h"
#include "ff/ff_event.h"
#include "ff/ff_stream.h"
#include "ff/ff_core.h"

/**
 * the maximum number of mrpc_client_request_processor instances, which can be simultaneously invoked
 * by the current mrpc_client_stream_processor.
 * This number should be less or equal to the 0x100, because of restriction on the size (1 bytes) of request_id
 * field in the mrpc client-server protocol. request_id is assigned for each currently running request
 * and is used for association with the corresponding asynchronous response.
 * So there are 0x100 maximum request processors can be invoked in parallel.
 */
#define MAX_REQUEST_PROCESSORS_CNT 0x100

/**
 * the maximum number of mrpc_packet packets, which can be used by the instance of the
 * mrpc_client_stream_processor. These packets are used when receiving data from the underlying stream.
 * Also they are used by the mrpc_client_request_processor when serializing rpc requests.
 * In order to avoid deadlocks this number must be not less than (2 * MAX_REQUEST_PROCESSORS_CNT).
 */
#define MAX_PACKETS_CNT (2 * MAX_REQUEST_PROCESSORS_CNT)

enum client_stream_processor_state
{
	STATE_WORKING,
	STATE_STOP_INITIATED,
	STATE_STOPPED,
};

struct mrpc_client_stream_processor
{
	struct ff_blocking_queue *writer_queue;
	struct ff_event *writer_stop_event;
	struct mrpc_bitmap *request_processors_bitmap;
	struct ff_pool *request_processors_pool;
	struct ff_event *request_processors_stop_event;
	struct ff_pool *packets_pool;
	struct mrpc_client_request_processor **active_request_processors;
	struct ff_stream *stream;
	int active_request_processors_cnt;
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
	request_id = mrpc_bitmap_acquire_bit(stream_processor->request_processors_bitmap);
	ff_assert(request_id >= 0);
	ff_assert(request_id < MAX_REQUEST_PROCESSORS_CNT);

	return (uint8_t) request_id;
}

static void release_request_id(void *ctx, uint8_t request_id)
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	mrpc_bitmap_release_bit(stream_processor->request_processors_bitmap, request_id);
}

static void *create_request_processor(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_client_request_processor *request_processor;
	uint8_t request_id;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	ff_assert(stream_processor->state != STATE_STOPPED);

	request_id = acquire_request_id(stream_processor);
	/* The current stream processor can simultaneously use up to MAX_PACKETS_CNT packets.
	 * So, it will be safe to set the reader queue's size to MAX_PACKETS_CNT.
	 */
	request_processor = mrpc_client_request_processor_create(acquire_packet, release_packet, stream_processor, release_request_id, stream_processor,
		stream_processor->writer_queue, MAX_PACKETS_CNT, request_id);
	return request_processor;
}

static void delete_request_processor(void *ctx)
{
	struct mrpc_client_request_processor *request_processor;

	request_processor = (struct mrpc_client_request_processor *) ctx;
	mrpc_client_request_processor_delete(request_processor);
}

static struct mrpc_client_request_processor *acquire_request_processor(struct mrpc_client_stream_processor *stream_processor)
{
	struct mrpc_client_request_processor *request_processor;
	uint8_t request_id;

	ff_assert(stream_processor->active_request_processors_cnt >= 0);
	ff_assert(stream_processor->active_request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	ff_assert(stream_processor->state != STATE_STOPPED);

	request_processor = (struct mrpc_client_request_processor *) ff_pool_acquire_entry(stream_processor->request_processors_pool);
	request_id = mrpc_client_request_processor_get_request_id(request_processor);
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

static void release_request_processor(struct mrpc_client_stream_processor *stream_processor, struct mrpc_client_request_processor *request_processor)
{
	uint8_t request_id;

	ff_assert(stream_processor->active_request_processors_cnt > 0);
	ff_assert(stream_processor->active_request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	ff_assert(stream_processor->state != STATE_STOPPED);

	request_id = mrpc_client_request_processor_get_request_id(request_processor);
	ff_assert(stream_processor->active_request_processors[request_id] == request_processor);
	stream_processor->active_request_processors[request_id] = NULL;
	ff_pool_release_entry(stream_processor->request_processors_pool, request_processor);

	stream_processor->active_request_processors_cnt--;
	if (stream_processor->active_request_processors_cnt == 0)
	{
		ff_event_set(stream_processor->request_processors_stop_event);
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

static void stop_all_request_processors(struct mrpc_client_stream_processor *stream_processor)
{
	struct mrpc_client_request_processor **active_request_processors;
	int i;

	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	active_request_processors = stream_processor->active_request_processors;
	for (i = 0; i < MAX_REQUEST_PROCESSORS_CNT; i++)
	{
		struct mrpc_client_request_processor *request_processor;

		request_processor = active_request_processors[i];
		if (request_processor != NULL)
		{
			mrpc_client_request_processor_stop_async(request_processor);
		}
	}
	ff_event_wait(stream_processor->request_processors_stop_event);
	ff_assert(stream_processor->active_request_processors_cnt == 0);
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

struct mrpc_client_stream_processor *mrpc_client_stream_processor_create()
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ff_malloc(sizeof(*stream_processor));

	/* the maximum number of packets in the writer_queue is limited by the packets_pool size (MAX_PACKETS_CNT),
	 * because only packets from those pool can be used by the stream_processor.
	 */
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_PACKETS_CNT);
	stream_processor->writer_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_processors_bitmap = mrpc_bitmap_create(MAX_REQUEST_PROCESSORS_CNT);
	stream_processor->request_processors_pool = ff_pool_create(MAX_REQUEST_PROCESSORS_CNT, create_request_processor, stream_processor, delete_request_processor);
	stream_processor->request_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);
	stream_processor->active_request_processors = (struct mrpc_client_request_processor **) ff_calloc(MAX_REQUEST_PROCESSORS_CNT, sizeof(stream_processor->active_request_processors[0]));

	stream_processor->stream = NULL;
	stream_processor->active_request_processors_cnt = 0;
	stream_processor->state = STATE_STOPPED;

	return stream_processor;
}

void mrpc_client_stream_processor_delete(struct mrpc_client_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->active_request_processors_cnt == 0);
	/* state can be either STATE_STOPPED either STATE_STOP_INITIATED.
	 * The second case is possible if the mrpc_client_stream_processor_stop_async()
	 * was called when the state was STATE_STOPPED and the mrpc_client_stream_processor_process_stream() wasn't called
	 * after the mrpc_client_stream_processor_stop_async() call.
	 */
	ff_assert(stream_processor->state != STATE_WORKING);

	ff_free(stream_processor->active_request_processors);
	ff_pool_delete(stream_processor->packets_pool);
	ff_event_delete(stream_processor->request_processors_stop_event);
	ff_pool_delete(stream_processor->request_processors_pool);
	mrpc_bitmap_delete(stream_processor->request_processors_bitmap);
	ff_event_delete(stream_processor->writer_stop_event);
	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_free(stream_processor);
}

void mrpc_client_stream_processor_process_stream(struct mrpc_client_stream_processor *stream_processor, struct ff_stream *stream)
{
	struct mrpc_client_request_processor **active_request_processors;

	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->active_request_processors_cnt == 0);
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
	ff_event_set(stream_processor->request_processors_stop_event);
	active_request_processors = stream_processor->active_request_processors;
	for (;;)
	{
		struct mrpc_packet *packet;
		struct mrpc_client_request_processor *request_processor;
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
		request_processor = active_request_processors[request_id];
		if (request_processor == NULL)
		{
			ff_log_debug(L"there is no active request_processor for the request_id=%lu read from the stream=%p", (uint32_t) request_id, stream);
			release_client_packet(stream_processor, packet);
			break;
		}
		mrpc_client_request_processor_push_packet(request_processor, packet);
	}
	mrpc_client_stream_processor_stop_async(stream_processor);
	ff_assert(stream_processor->state == STATE_STOP_INITIATED);
	stop_all_request_processors(stream_processor);
	ff_assert(stream_processor->active_request_processors_cnt == 0);
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
		ff_assert(stream_processor->active_request_processors_cnt == 0);
		stream_processor->state = STATE_STOP_INITIATED;
	}
	else
	{
		ff_assert(stream_processor->state == STATE_STOP_INITIATED);
		ff_log_debug(L"the stream_processor=%p didn't yet stopped, so do nothing", stream_processor);
	}
}

enum ff_result mrpc_client_stream_processor_invoke_rpc(struct mrpc_client_stream_processor *stream_processor, struct mrpc_data *data)
{
	enum ff_result result = FF_FAILURE;

	if (stream_processor->state == STATE_WORKING)
	{
		struct mrpc_client_request_processor *request_processor;

		request_processor = acquire_request_processor(stream_processor);
		result = mrpc_client_request_processor_invoke_rpc(request_processor, data);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot invoke rpc for data=%p on the request_processor=%p. See previous messages for more info", data, request_processor);
			mrpc_client_stream_processor_stop_async(stream_processor);
		}
		release_request_processor(stream_processor, request_processor);
	}
	else
	{
		ff_log_debug(L"the stream_processor=%p cannot invoke rpc (data=%p), because it is stopped or its stop proces is initiated", stream_processor, data);
	}

	return result;
}
