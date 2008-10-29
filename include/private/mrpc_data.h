#ifndef MRPC_DATA_PRIVATE
#define MRPC_DATA_PRIVATE

#include "mrpc/mrpc_data.h"
#include "private/mrpc_interface.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ff_result mrpc_data_process_remote_call(struct mrpc_interface *interface, void *service_ctx, struct ff_stream *stream);

enum ff_result mrpc_data_invoke_remote_call(struct mrpc_data *data, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
