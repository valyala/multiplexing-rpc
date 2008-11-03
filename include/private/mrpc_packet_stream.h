#ifndef MRPC_PACKET_STREAM_PRIVATE
#define MRPC_PACKET_STREAM_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_packet.h"
#include "ff/ff_blocking_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This callback must return an empty mrpc_packet.
 * It is expected that the callback always return correct result.
 */
typedef struct mrpc_packet *(*mrpc_packet_stream_acquire_packet_func)(void *packet_func_ctx);

/**
 * This callback must release the given packet, which was acquired using
 * the mrpc_packet_stream_acquire_packet_func() callback.
 */
typedef void (*mrpc_packet_stream_release_packet_func)(void *packet_func_ctx, struct mrpc_packet *packet);

struct mrpc_packet_stream;

/**
 * Creates packet stream.
 * writer_queue is used for pushing packets written by the mrpc_packet_stream_write() and mrpc_packet_stream_flush().
 * packets from the writer_queue must be deleted using the same technique as used by
 * the mrpc_packet_stream_release_packet_func() callback.
 * acquire_packet_func and release_packet_func are used for acquiring packets for pushing them into the writer_queue
 * and releasing packets pushed to the mrpc_packet_stream_push_packet() function.
 * Always returns correct result.
 */
struct mrpc_packet_stream *mrpc_packet_stream_create(struct ff_blocking_queue *writer_queue,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx);

/**
 * Deletes the given stream.
 */
void mrpc_packet_stream_delete(struct mrpc_packet_stream *stream);

/**
 * Initializes the given stream and associates it with the given request_id.
 * This function must be called before starting reading / writing to the stream.
 * It is usually called after mrpc_packet_stream_create() or mrpc_packet_stream_shutdown() calls.
 */
void mrpc_packet_stream_initialize(struct mrpc_packet_stream *stream, uint8_t request_id);

/**
 * Shutdowns the given stream.
 * After the stream has been shutted down, it can be initialized again
 * using the mrpc_packet_stream_initialize().
 */
void mrpc_packet_stream_shutdown(struct mrpc_packet_stream *stream);

/**
 * Reads exactly len bytes from the stream into the buf.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_packet_stream_read(struct mrpc_packet_stream *stream, void *buf, int len);

/**
 * Writes exactly len bytes from the buf into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_packet_stream_write(struct mrpc_packet_stream *stream, const void *buf, int len);

/**
 * Flushes the write buffer of the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_packet_stream_flush(struct mrpc_packet_stream *stream);

/**
 * Unblocks pending and subsequent mrpc_packet_stream_read() calls.
 */
void mrpc_packet_stream_disconnect(struct mrpc_packet_stream *stream);

/**
 * Pushes the given packet to the reader queue of the given stream.
 * The packet must be allocated using the same technique as used by the mrpc_packet_stream_acquire_packet_func() callback
 * passed to the mrpc_packet_stream_create() function.
 */
void mrpc_packet_stream_push_packet(struct mrpc_packet_stream *stream, struct mrpc_packet *packet);

#ifdef __cplusplus
}
#endif

#endif
