#ifndef MRPC_DATA_PRIVATE_H
#define MRPC_DATA_PRIVATE_H

#include "mrpc/mrpc_data.h"
#include "private/mrpc_interface.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a mrpc_data for the given method_id from the interface.
 * method_id must be valid id of the method in the given interface.
 * Always returns correct result.
 */
struct mrpc_data *mrpc_data_create(const struct mrpc_interface *interface, uint8_t method_id);

/**
 * receive the next remote call from the stream and process it according to the interface.
 * service_ctx is passed to the rpc callback.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_data_process_remote_call(const struct mrpc_interface *interface, void *service_ctx, struct ff_stream *stream);

/**
 * sends the rpc request from the data to the stream and receives corresponding response from the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_data_invoke_remote_call(struct mrpc_data *data, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
