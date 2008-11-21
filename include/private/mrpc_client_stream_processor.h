#ifndef MRPC_CLIENT_STREAM_PROCESSOR_PRIVATE_H
#define MRPC_CLIENT_STREAM_PROCESSOR_PRIVATE_H

#include "private/mrpc_common.h"
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
 * Creates stream for sending rpc requests.
 * This stream must be deleted using ff_stream_delete().
 * Returns request stream on success, NULL on error.
 */
struct ff_stream *mrpc_client_stream_processor_create_request_stream(struct mrpc_client_stream_processor *stream_processor);

#ifdef __cplusplus
}
#endif

#endif
