#ifndef MRPC_CLIENT_STREAM_PROCESSOR_PRIVATE
#define MRPC_CLIENT_STREAM_PROCESSOR_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_data.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_client_stream_processor;

struct mrpc_client_stream_processor *mrpc_client_stream_processor_create();

void mrpc_client_stream_processor_delete(struct mrpc_client_stream_processor *stream_processor);

void mrpc_client_stream_processor_process_stream(struct mrpc_client_stream_processor *stream_processor, struct ff_stream *stream);

void mrpc_client_stream_processor_stop_async(struct mrpc_client_stream_processor *stream_processor);

enum ff_result mrpc_client_stream_processor_invoke_rpc(struct mrpc_client_stream_processor *stream_processor, struct mrpc_data *data);

#ifdef __cplusplus
}
#endif

#endif
