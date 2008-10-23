#ifndef MRPC_SERVER_PUBLIC
#define MRPC_SERVER_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_interface.h"
#include "ff/ff_endpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_server;

MRPC_API struct mrpc_server *mrpc_server_create(struct mrpc_interface *service_interface, void *service_ctx, struct ff_endpoint *endpoint);

MRPC_API void mrpc_server_delete(struct mrpc_server *server);

MRPC_API void mrpc_server_start(struct mrpc_server *server);

MRPC_API void mrpc_server_stop(struct mrpc_server *server);

#ifdef __cplusplus
}
#endif

#endif
