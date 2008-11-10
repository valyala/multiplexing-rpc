#include "private/mrpc_common.h"

#include "private/mrpc_packet_stream.h"
#include "private/mrpc_packet.h"
#include "ff/ff_blocking_queue.h"

/**
 * the maximum size of mrpc_packet packets pending in the reader_queue.
 * these packets are consumed by the mrpc_packet_stream_read() function.
 * TODO: determine optimal value for this parameter.
 */
#define MAX_READER_QUEUE_SIZE 100

/**
 * Timeout (in milliseconds) for the mrpc_packet_stream_read() function.
 * TODO: determine optimal value for this parameter.
 */
#define READ_TIMEOUT 2000

/**
 * Timeout (in milliseconds) for the mrpc_packet_stream_write() and
 * mrpc_packet_stream_flush() functions.
 * TODO: determine optimal value for this parameter.
 */
#define WRITE_TIMEOUT 2000

struct mrpc_packet_stream
{
	mrpc_packet_stream_acquire_packet_func acquire_packet_func;
	mrpc_packet_stream_release_packet_func release_packet_func;
	void *packet_func_ctx;
	struct ff_blocking_queue *reader_queue;
	struct ff_blocking_queue *writer_queue;
	struct mrpc_packet *current_read_packet;
	struct mrpc_packet *current_write_packet;
	uint8_t request_id;
};

static struct mrpc_packet *acquire_packet(struct mrpc_packet_stream *stream, enum mrpc_packet_type packet_type)
{
	struct mrpc_packet *packet;

	ff_assert(packet_type != MRPC_PACKET_SINGLE);
	packet = stream->acquire_packet_func(stream->packet_func_ctx);
	mrpc_packet_set_request_id(packet, stream->request_id);
	mrpc_packet_set_type(packet, packet_type);

	return packet;
}

static void release_packet(struct mrpc_packet_stream *stream, struct mrpc_packet *packet)
{
	stream->release_packet_func(stream->packet_func_ctx, packet);
}

static enum ff_result prefetch_current_read_packet(struct mrpc_packet_stream *stream)
{
	enum ff_result result;

	ff_assert(stream->current_read_packet == NULL);
	result = ff_blocking_queue_get_with_timeout(stream->reader_queue, (const void **) &stream->current_read_packet, READ_TIMEOUT);
	if (result == FF_SUCCESS)
	{
		enum mrpc_packet_type packet_type;

		ff_assert(stream->current_read_packet != NULL);
		packet_type = mrpc_packet_get_type(stream->current_read_packet);
		if (packet_type != MRPC_PACKET_START && packet_type != MRPC_PACKET_SINGLE)
		{
			release_packet(stream, stream->current_read_packet);
			stream->current_read_packet = NULL;
			result = FF_FAILURE;
		}
	}
	else
	{
		ff_assert(stream->current_read_packet == NULL);
	}

	return result;
}

static void release_current_read_packet(struct mrpc_packet_stream *stream)
{
	if (stream->current_read_packet != NULL)
	{
		release_packet(stream, stream->current_read_packet);
		stream->current_read_packet = NULL;
	}
}

static void acquire_current_write_packet(struct mrpc_packet_stream *stream)
{
	ff_assert(stream->current_write_packet == NULL);
	stream->current_write_packet = acquire_packet(stream, MRPC_PACKET_START);
}

static void release_current_write_packet(struct mrpc_packet_stream *stream)
{
	if (stream->current_write_packet != NULL)
	{
		enum mrpc_packet_type packet_type;

		packet_type = mrpc_packet_get_type(stream->current_write_packet);
		ff_assert(packet_type == MRPC_PACKET_END);
		release_packet(stream, stream->current_write_packet);
		stream->current_write_packet = NULL;
	}
}

static void clear_reader_queue(struct mrpc_packet_stream *stream)
{
	for (;;)
	{
		struct mrpc_packet *packet;
		int is_empty;

		is_empty = ff_blocking_queue_is_empty(stream->reader_queue);
		if (is_empty)
		{
			break;
		}
		ff_blocking_queue_get(stream->reader_queue, (const void **) &packet);
		release_packet(stream, packet);
	}
}

struct mrpc_packet_stream *mrpc_packet_stream_create(struct ff_blocking_queue *writer_queue,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx)
{
	struct mrpc_packet_stream *stream;

	ff_assert(acquire_packet_func != NULL);
	ff_assert(release_packet_func != NULL);
	ff_assert(writer_queue != NULL);

	stream = (struct mrpc_packet_stream *) ff_malloc(sizeof(*stream));
	stream->acquire_packet_func = acquire_packet_func;
	stream->release_packet_func = release_packet_func;
	stream->packet_func_ctx = packet_func_ctx;
	stream->reader_queue = ff_blocking_queue_create(MAX_READER_QUEUE_SIZE);
	stream->writer_queue = writer_queue;
	stream->current_read_packet = NULL;
	stream->current_write_packet = NULL;
	stream->request_id = 0;

	return stream;
}

void mrpc_packet_stream_delete(struct mrpc_packet_stream *stream)
{
	ff_assert(stream->current_read_packet == NULL);
	ff_assert(stream->current_write_packet == NULL);
	ff_assert(stream->request_id == 0);

	ff_blocking_queue_delete(stream->reader_queue);
	ff_free(stream);
}

void mrpc_packet_stream_initialize(struct mrpc_packet_stream *stream, uint8_t request_id)
{
	ff_assert(stream->current_read_packet == NULL);
	ff_assert(stream->current_write_packet == NULL);
	ff_assert(stream->request_id == 0);

	stream->request_id = request_id;
}

void mrpc_packet_stream_shutdown(struct mrpc_packet_stream *stream)
{
	mrpc_packet_stream_flush(stream);
	release_current_write_packet(stream);
	ff_assert(stream->current_write_packet == NULL);
	release_current_read_packet(stream);
	ff_assert(stream->current_read_packet == NULL);
	clear_reader_queue(stream);

	stream->request_id = 0;
}

enum ff_result mrpc_packet_stream_read(struct mrpc_packet_stream *stream, void *buf, int len)
{
	char *p;
	enum mrpc_packet_type packet_type;
	enum ff_result result = FF_FAILURE;

	ff_assert(len >= 0);

	if (stream->current_read_packet == NULL)
	{
		/* this is the first call of the mrpc_packet_stream_read(), so try to prefetch the current read packet */
		result = prefetch_current_read_packet(stream);
		if (result != FF_SUCCESS)
		{
			goto end;
		}
		else
		{
			ff_assert(stream->current_read_packet != NULL);
		}
	}

	p = (char *) buf;
	packet_type = mrpc_packet_get_type(stream->current_read_packet);
	while (len > 0)
	{
		int bytes_read;

		bytes_read = mrpc_packet_read_data(stream->current_read_packet, p, len);
		ff_assert(bytes_read >= 0);
		ff_assert(bytes_read <= len);
		len -= bytes_read;
		p += bytes_read;

        if (len > 0)
		{
			struct mrpc_packet *packet;

			if (packet_type == MRPC_PACKET_SINGLE || packet_type == MRPC_PACKET_END)
			{
				/* error: the previous packet should be the last in the stream,
				 * but we didn't read requested len bytes.
				 */
				result = FF_FAILURE;
				goto end;
			}

			result = ff_blocking_queue_get_with_timeout(stream->reader_queue, (const void **) &packet, READ_TIMEOUT);
			if (result != FF_SUCCESS)
			{
				goto end;
			}
			ff_assert(packet != NULL);
			packet_type = mrpc_packet_get_type(packet);
			if (packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_SINGLE)
			{
				/* wrong packet type. */
				release_packet(stream, packet);
				result = FF_FAILURE;
				goto end;
			}

			release_packet(stream, stream->current_read_packet);
			stream->current_read_packet = packet;
		}
	}
	result = FF_SUCCESS;

end:
	return result;
}

enum ff_result mrpc_packet_stream_write(struct mrpc_packet_stream *stream, const void *buf, int len)
{
	const char *p;
	enum mrpc_packet_type packet_type;
	enum ff_result result = FF_FAILURE;

	ff_assert(len >= 0);

	if (stream->current_write_packet == NULL)
	{
		/* this is the first call of the mrpc_packet_stream_write() function. So acquire the writer packet */
		acquire_current_write_packet(stream);
		ff_assert(stream->current_write_packet != NULL);
	}

	p = (const char *) buf;
	packet_type = mrpc_packet_get_type(stream->current_write_packet);
	if (packet_type == MRPC_PACKET_END)
	{
		/* the stream was already flushed, so it is impossible to write more data to it */
		goto end;
	}
	ff_assert(packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_MIDDLE);
	while (len > 0)
	{
		int bytes_written;

		bytes_written = mrpc_packet_write_data(stream->current_write_packet, p, len);
		ff_assert(bytes_written >= 0);
		ff_assert(bytes_written <= len);
		len -= bytes_written;
		p += bytes_written;

		if (len > 0)
		{
			result = ff_blocking_queue_put_with_timeout(stream->writer_queue, stream->current_write_packet, WRITE_TIMEOUT);
			if (result != FF_SUCCESS)
			{
				goto end;
			}
			stream->current_write_packet = acquire_packet(stream, MRPC_PACKET_MIDDLE);
		}
	}
	result = FF_SUCCESS;

end:
	return result;
}

enum ff_result mrpc_packet_stream_flush(struct mrpc_packet_stream *stream)
{
	enum mrpc_packet_type packet_type;
	enum ff_result result = FF_SUCCESS;

	if (stream->current_write_packet == NULL)
	{
		/* the mrpc_packet_stream_write() function wasn't called at all,
		 * so there is no need to flush the stream
		 */
		goto end;
	}

	packet_type = mrpc_packet_get_type(stream->current_write_packet);
	if (packet_type == MRPC_PACKET_END)
	{
		/* the stream was already flushed */
		goto end;
	}

	ff_assert(packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_MIDDLE);
	if (packet_type == MRPC_PACKET_START)
	{
		packet_type = MRPC_PACKET_SINGLE;
	}
	else
	{
		packet_type = MRPC_PACKET_END;
	}
	mrpc_packet_set_type(stream->current_write_packet, packet_type);
	result = ff_blocking_queue_put_with_timeout(stream->writer_queue, stream->current_write_packet, WRITE_TIMEOUT);
	if (result != FF_SUCCESS)
	{
		release_packet(stream, stream->current_write_packet);
	}
	stream->current_write_packet = acquire_packet(stream, MRPC_PACKET_END);

end:
	return result;
}

void mrpc_packet_stream_disconnect(struct mrpc_packet_stream *stream)
{
	struct mrpc_packet *packet;

	mrpc_packet_stream_flush(stream);
	packet = acquire_packet(stream, MRPC_PACKET_END);
	ff_blocking_queue_put(stream->reader_queue, packet);
}

void mrpc_packet_stream_push_packet(struct mrpc_packet_stream *stream, struct mrpc_packet *packet)
{
	ff_assert(packet != NULL);

	ff_blocking_queue_put(stream->reader_queue, packet);
}
