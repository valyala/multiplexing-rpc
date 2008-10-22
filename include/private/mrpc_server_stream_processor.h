#ifndef MRPC_SERVER_STREAM_PROCESSOR_PRIVATE
#define MRPC_SERVER_STREAM_PROCESSOR_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_interface.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_server_stream_processor;

typedef void (*mrpc_server_stream_processor_release_func)(void *release_func_ctx, struct mrpc_server_stream_processor *stream_processor);

struct mrpc_server_stream_processor *mrpc_server_stream_processor_create(mrpc_server_stream_processor_release_func release_func,
	void *release_func_ctx, struct mrpc_interface *service_interface, void *service_ctx);

void mrpc_server_stream_processor_delete(struct mrpc_server_stream_processor *stream_processor);

void mrpc_server_stream_processor_start(struct mrpc_server_stream_processor *stream_processor, struct ff_stream *stream);

void mrpc_server_stream_processor_stop_async(struct mrpc_server_stream_processor *stream_processor);

#ifdef __cplusplus
}
#endif

#endif
