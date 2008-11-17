#include "private/mrpc_common.h"

#include "private/mrpc_packet_stream.h"
#include "private/mrpc_packet.h"
#include "ff/ff_blocking_queue.h"

/**
 * Timeout (in milliseconds) for the mrpc_packet_stream_read() function.
 * This timeout prevents DoS from malicious peers, which don't send packets
 * to the stream's reader_queue, so blocking the mrpc_packet_stream_read() callers forever,
 * which can lead to workers shortage.
 * This timeout shouldn't be small, because this can lead to frequent failures of the mrpc_packet_stream_read(),
 * because it won't wait for the next data packet.
 */
#define READ_TIMEOUT (120 * 1000)

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
	struct mrpc_packet *current_read_packet;
	struct ff_blocking_queue *reader_queue;
	enum ff_result result;

	ff_assert(stream->current_read_packet == NULL);
	reader_queue = stream->reader_queue;
	result = ff_blocking_queue_get_with_timeout(reader_queue, (const void **) &current_read_packet, READ_TIMEOUT);
	if (result == FF_SUCCESS)
	{
		enum mrpc_packet_type packet_type;

		ff_assert(current_read_packet != NULL);
		packet_type = mrpc_packet_get_type(current_read_packet);
		if (packet_type != MRPC_PACKET_START && packet_type != MRPC_PACKET_SINGLE)
		{
			ff_log_debug(L"wrong packet_type=%d of the first packet in the packet stream=%p", (int) packet_type, stream);
			release_packet(stream, current_read_packet);
			result = FF_FAILURE;
		}
		else
		{
			stream->current_read_packet = current_read_packet;
		}
	}
	else
	{
		ff_log_debug(L"cannot get next packet from the reader_queue=%p during the timeout=%d", reader_queue, READ_TIMEOUT);
	}

	return result;
}

static void release_current_read_packet(struct mrpc_packet_stream *stream)
{
	struct mrpc_packet *current_read_packet;

	current_read_packet = stream->current_read_packet;
	if (current_read_packet != NULL)
	{
		release_packet(stream, current_read_packet);
		stream->current_read_packet = NULL;
	}
	else
	{
		ff_log_debug(L"the packet stream=%p has been shutdowned without reading from it", stream);
	}
}

static void acquire_current_write_packet(struct mrpc_packet_stream *stream)
{
	ff_assert(stream->current_write_packet == NULL);
	stream->current_write_packet = acquire_packet(stream, MRPC_PACKET_START);
}

static void release_current_write_packet(struct mrpc_packet_stream *stream)
{
	struct mrpc_packet *current_write_packet;

	current_write_packet = stream->current_write_packet;
	if (current_write_packet != NULL)
	{
		enum mrpc_packet_type packet_type;

		packet_type = mrpc_packet_get_type(current_write_packet);
		ff_assert(packet_type != MRPC_PACKET_SINGLE);
		if (packet_type != MRPC_PACKET_END)
		{
			ff_log_debug(L"the packet stream=%p has been shutdowned without previous flushing. packet_type=%d", stream, (int) packet_type);
		}
		release_packet(stream, current_write_packet);
		stream->current_write_packet = NULL;
	}
	else
	{
		ff_log_debug(L"the packet stream=%p has been shutdowned without writing to it", stream);
	}
}

static void clear_reader_queue(struct mrpc_packet_stream *stream)
{
	struct ff_blocking_queue *reader_queue;

	reader_queue = stream->reader_queue;
	for (;;)
	{
		struct mrpc_packet *packet;
		int is_empty;

		is_empty = ff_blocking_queue_is_empty(reader_queue);
		if (is_empty)
		{
			break;
		}
		ff_blocking_queue_get(reader_queue, (const void **) &packet);
		release_packet(stream, packet);
	}
}

struct mrpc_packet_stream *mrpc_packet_stream_create(struct ff_blocking_queue *writer_queue, int max_reader_queue_size,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx)
{
	struct mrpc_packet_stream *stream;

	ff_assert(acquire_packet_func != NULL);
	ff_assert(release_packet_func != NULL);
	ff_assert(writer_queue != NULL);
	ff_assert(max_reader_queue_size > 0);

	stream = (struct mrpc_packet_stream *) ff_malloc(sizeof(*stream));
	stream->acquire_packet_func = acquire_packet_func;
	stream->release_packet_func = release_packet_func;
	stream->packet_func_ctx = packet_func_ctx;
	stream->reader_queue = ff_blocking_queue_create(max_reader_queue_size);
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
	release_current_write_packet(stream);
	ff_assert(stream->current_write_packet == NULL);
	release_current_read_packet(stream);
	ff_assert(stream->current_read_packet == NULL);
	clear_reader_queue(stream);

	stream->request_id = 0;
}

enum ff_result mrpc_packet_stream_read(struct mrpc_packet_stream *stream, void *buf, int len)
{
	struct mrpc_packet *current_read_packet;
	char *p;
	enum mrpc_packet_type packet_type;
	enum ff_result result = FF_FAILURE;

	ff_assert(len >= 0);

	current_read_packet = stream->current_read_packet;
	if (current_read_packet == NULL)
	{
		/* this is the first call of the mrpc_packet_stream_read(), so try to prefetch the current read packet */
		result = prefetch_current_read_packet(stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot prefetch the first read packet for the packet stream=%p. See previous messages for more info", stream);
			goto end;
		}
		else
		{
			ff_assert(stream->current_read_packet != NULL);
			current_read_packet = stream->current_read_packet;
		}
	}

	p = (char *) buf;
	packet_type = mrpc_packet_get_type(current_read_packet);
	while (len > 0)
	{
		int bytes_read;

		bytes_read = mrpc_packet_read_data(current_read_packet, p, len);
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
				ff_log_debug(L"the packet stream=%p has been finished, but len=%d bytes yet not read to the buf=%p", stream, len, buf);
				result = FF_FAILURE;
				goto end;
			}

			result = ff_blocking_queue_get_with_timeout(stream->reader_queue, (const void **) &packet, READ_TIMEOUT);
			if (result != FF_SUCCESS)
			{
				ff_log_debug(L"cannot get the next packet from the reader_queue=%d during the timeout=%d", stream->reader_queue, READ_TIMEOUT);
				goto end;
			}
			ff_assert(packet != NULL);
			packet_type = mrpc_packet_get_type(packet);
			if (packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_SINGLE)
			{
				/* wrong packet type. */
				ff_log_debug(L"packet with wrong type=%d has been received from the packet stream=%p", (int) packet_type, stream);
				release_packet(stream, packet);
				result = FF_FAILURE;
				goto end;
			}

			release_packet(stream, current_read_packet);
			current_read_packet = packet;
		}
	}
	result = FF_SUCCESS;

end:
	stream->current_read_packet = current_read_packet;
	return result;
}

enum ff_result mrpc_packet_stream_write(struct mrpc_packet_stream *stream, const void *buf, int len)
{
	struct mrpc_packet *current_write_packet;
	struct ff_blocking_queue *writer_queue;
	const char *p;
	enum mrpc_packet_type packet_type;

	ff_assert(len >= 0);
	current_write_packet = stream->current_write_packet;
	if (current_write_packet == NULL)
	{
		/* this is the first call of the mrpc_packet_stream_write() function. So acquire the writer packet */
		acquire_current_write_packet(stream);
		ff_assert(stream->current_write_packet != NULL);
		current_write_packet = stream->current_write_packet;
	}

	p = (const char *) buf;
	packet_type = mrpc_packet_get_type(current_write_packet);
	ff_assert(packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_MIDDLE);
	writer_queue = stream->writer_queue;
	while (len > 0)
	{
		int bytes_written;

		bytes_written = mrpc_packet_write_data(current_write_packet, p, len);
		ff_assert(bytes_written >= 0);
		ff_assert(bytes_written <= len);
		len -= bytes_written;
		p += bytes_written;

		if (len > 0)
		{
			ff_blocking_queue_put(writer_queue, current_write_packet);
			current_write_packet = acquire_packet(stream, MRPC_PACKET_MIDDLE);
		}
	}

	stream->current_write_packet = current_write_packet;
	return FF_SUCCESS;
}

enum ff_result mrpc_packet_stream_flush(struct mrpc_packet_stream *stream)
{
	struct mrpc_packet *current_write_packet;
	enum mrpc_packet_type packet_type;

	ff_assert(stream->current_write_packet != NULL);
	current_write_packet = stream->current_write_packet;

	packet_type = mrpc_packet_get_type(current_write_packet);
	ff_assert(packet_type == MRPC_PACKET_START || packet_type == MRPC_PACKET_MIDDLE);
	if (packet_type == MRPC_PACKET_START)
	{
		packet_type = MRPC_PACKET_SINGLE;
	}
	else
	{
		packet_type = MRPC_PACKET_END;
	}
	mrpc_packet_set_type(current_write_packet, packet_type);
	ff_blocking_queue_put(stream->writer_queue, current_write_packet);
	stream->current_write_packet = acquire_packet(stream, MRPC_PACKET_END);

	return FF_SUCCESS;
}

void mrpc_packet_stream_disconnect(struct mrpc_packet_stream *stream)
{
	struct mrpc_packet *packet;

	packet = acquire_packet(stream, MRPC_PACKET_END);
	ff_blocking_queue_put(stream->reader_queue, packet);
}

void mrpc_packet_stream_push_packet(struct mrpc_packet_stream *stream, struct mrpc_packet *packet)
{
	ff_assert(packet != NULL);

	ff_blocking_queue_put(stream->reader_queue, packet);
}
