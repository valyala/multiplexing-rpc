#ifndef MRPC_PACKET_STREAM_PRIVATE
#define MRPC_PACKET_STREAM_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_packet.h"
#include "ff/ff_blocking_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mrpc_packet *(*mrpc_packet_stream_acquire_packet_func)(void *packet_func_ctx);
typedef void (*mrpc_packet_stream_release_packet_func)(void *packet_func_ctx, struct mrpc_packet *packet);

struct mrpc_packet_stream;

struct mrpc_packet_stream *mrpc_packet_stream_create(struct ff_blocking_queue *writer_queue,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx);

void mrpc_packet_stream_delete(struct mrpc_packet_stream *stream);

int mrpc_packet_stream_initialize(struct mrpc_packet_stream *stream, uint8_t request_id);

int mrpc_packet_stream_shutdown(struct mrpc_packet_stream *stream);

void mrpc_packet_stream_clear_reader_queue(struct mrpc_packet_stream *stream);

/**
 * Reads exactly len bytes from the stream into the buf.
 * Returns 1 on success, 0 on error.
 */
int mrpc_packet_stream_read(struct mrpc_packet_stream *stream, void *buf, int len);

/**
 * Writes exactly len bytes from the buf into the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_packet_stream_write(struct mrpc_packet_stream *stream, const void *buf, int len);

/**
 * Flushes the write buffer of the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_packet_stream_flush(struct mrpc_packet_stream *stream);

void mrpc_packet_stream_disconnect(struct mrpc_packet_stream *stream);

void mrpc_packet_stream_push_packet(struct mrpc_packet_stream *stream, struct mrpc_packet *packet);

#ifdef __cplusplus
}
#endif

#endif
