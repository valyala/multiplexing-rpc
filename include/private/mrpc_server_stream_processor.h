#ifndef MRPC_SERVER_STREAM_PROCESSOR_PRIVATE_H
#define MRPC_SERVER_STREAM_PROCESSOR_PRIVATE_H

#include "private/mrpc_common.h"
#include "private/mrpc_interface.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_server_stream_processor;

/**
 * The callback must release the given stream_processor.
 * This callback is called before the stream processor will stop.
 */
typedef void (*mrpc_server_stream_processor_release_func)(void *release_func_ctx, struct mrpc_server_stream_processor *stream_processor);

/**
 * the callback must relase the given id.
 * This callback is called before the stream processor will be deleted.
 */
typedef void (*mrpc_server_stream_processor_release_id_func)(void *release_id_func_ctx, int id);

/**
 * Creates a stream processor.
 * release_func will be called when the given stream processor has been stopped.
 * release_id_func will be called at the beginning of the mrpc_server_stream_processor_delete()
 * Alwasy returns correct result.
 */
struct mrpc_server_stream_processor *mrpc_server_stream_processor_create(mrpc_server_stream_processor_release_func release_func, void *release_func_ctx,
	mrpc_server_stream_processor_release_id_func release_id_func, void *release_id_func_ctx, int id);

/**
 * Deletes the given stream_processor.
 * 
 */
void mrpc_server_stream_processor_delete(struct mrpc_server_stream_processor *stream_processor);

/**
 * Starts processing the given stream using the given stream_processor.
 * service_interface and service_ctx are used for invoking corresponding server callbacks.
 */
void mrpc_server_stream_processor_start(struct mrpc_server_stream_processor *stream_processor,
	struct mrpc_interface *service_interface, void *service_ctx, struct ff_stream *stream);

/**
 * Notifies the stream_processor to stop ASAP.
 * When the stream_processor will stop, it will call the release_func() callback
 * passed to the mrpc_server_stream_processor_create().
 * This function can be called multiple times and at any time while the stream_processor isn't deleted.
 * This function returns immediately.
 */
void mrpc_server_stream_processor_stop_async(struct mrpc_server_stream_processor *stream_processor);

/**
 * returns the id of the given stream processor.
 * This id is passed to the mrpc_server_stream_processor_create()
 */
int mrpc_server_stream_processor_get_id(struct mrpc_server_stream_processor *stream_processor);

#ifdef __cplusplus
}
#endif

#endif
