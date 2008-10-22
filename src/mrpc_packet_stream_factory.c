#include "mrpc/mrpc_common.h"

#include "private/mrpc_packet_stream_factory.h"
#include "private/mrpc_packet_stream.h"
#include "ff/ff_stream.h"

static void delete_packet_stream(struct ff_stream *stream)
{
	struct mrpc_packet_stream *packet_stream;

	packet_stream = (struct mrpc_packet_stream *) stream->ctx;
	mrpc_packet_stream_delete(packet_stream);
}

static enum ff_result read_from_packet_stream(struct ff_stream *stream, void *buf, int len)
{
	struct mrpc_packet_stream *packet_stream;
	enum ff_result result;

	ff_assert(len >= 0);

	packet_stream = (struct mrpc_packet_stream *) ff_stream_get_ctx(stream);
	result = mrpc_packet_stream_read(packet_stream, buf, len);
	return result;
}

static enum ff_result write_to_packet_stream(struct ff_stream *stream, const void *buf, int len)
{
	struct mrpc_packet_stream *packet_stream;
	enum ff_result result;

	ff_assert(len >= 0);

	packet_stream = (struct mrpc_packet_stream *) ff_stream_get_ctx(stream);
	result = mrpc_packet_stream_write(packet_stream, buf, len);
	return result;
}

static enum ff_result flush_packet_stream(struct ff_stream *stream)
{
	struct mrpc_packet_stream *packet_stream;
	enum ff_result result;

	packet_stream = (struct mrpc_packet_stream *) ff_stream_get_ctx(stream);
	result = mrpc_packet_stream_flush(packet_stream);
	return result;
}

static void disconnect_packet_stream(struct ff_stream *stream)
{
	struct mrpc_packet_stream *packet_stream;

	packet_stream = (struct mrpc_packet_stream *) stream->ctx;
	mrpc_packet_stream_disconnect(packet_stream);
}

static struct ff_stream_vtable packet_stream_vtable =
{
	delete_packet_stream,
	read_from_packet_stream,
	write_to_packet_stream,
	flush_packet_stream,
	disconnect_packet_stream
};

struct ff_stream *mrpc_packet_stream_factory_create_stream(struct mrpc_packet_stream *packet_stream)
{
	struct ff_stream *stream;

	stream = ff_stream_create(&packet_stream_vtable, packet_stream);
	return stream;
}
