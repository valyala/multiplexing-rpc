#ifndef MRPC_SERVER_STREAM_HANDLER_PUBLIC_H
#define MRPC_SERVER_STREAM_HANDLER_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This callback is called by the server in order to handle an rpc over the given stream.
 * service_ctx is the same, which was passed to the mrpc_server_start() function.
 */
typedef enum ff_result (*mrpc_server_stream_handler)(struct ff_stream *stream, void *service_ctx);

#ifdef __cplusplus
}
#endif

#endif
