#ifndef MRPC_CLIENT_STREAM_PROCESSOR_PRIVATE
#define MRPC_CLIENT_STREAM_PROCESSOR_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_data.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_client_stream_processor;

/**
 * Creates a client stream processor.
 * Always returns correct result.
 */
struct mrpc_client_stream_processor *mrpc_client_stream_processor_create();

/**
 * Deletes the given stream_processor.
 */
void mrpc_client_stream_processor_delete(struct mrpc_client_stream_processor *stream_processor);

/**
 * Processes the given stream using stream_processor.
 * This function returns only in two cases:
 * 1) mrpc_client_stream_processor_stop_async() was called;
 * 2) an error occured when processing the given stream.
 */
void mrpc_client_stream_processor_process_stream(struct mrpc_client_stream_processor *stream_processor, struct ff_stream *stream);

/**
 * Notifies the stream_processor, that it should be stopped.
 * This function is used for unblocking the mrpc_client_stream_processor_process_stream() function.
 * This function can be called multiple times and at any time while the stream_processor isn't deleted.
 * This function returns immediately.
 */
void mrpc_client_stream_processor_stop_async(struct mrpc_client_stream_processor *stream_processor);

/**
 * Invokes the given rpc data on the given stream_processor.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_client_stream_processor_invoke_rpc(struct mrpc_client_stream_processor *stream_processor, struct mrpc_data *data);

#ifdef __cplusplus
}
#endif

#endif
