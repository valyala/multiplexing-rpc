#ifndef MRPC_SERVER_PUBLIC
#define MRPC_SERVER_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_interface.h"
#include "ff/ff_endpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_server;

/**
 * Creates a rpc server.
 * The returned rpc server must be started using the mrpc_server_start() before it will process client's requests.
 * Always returns correct result.
 */
MRPC_API struct mrpc_server *mrpc_server_create();

/**
 * Deletes the given server.
 * The server must be stopped using the mrpc_server_stop() before it can be deleted.
 */
MRPC_API void mrpc_server_delete(struct mrpc_server *server);

/**
 * Starts the given server for serving the given service_interface and using the given service_ctx.
 */
MRPC_API void mrpc_server_start(struct mrpc_server *server, struct mrpc_interface *service_interface, void *service_ctx, struct ff_endpoint *endpoint);

/**
 * Stops the given server.
 * This function waits while the server will stop and returns only when the server is stopped.
 */
MRPC_API void mrpc_server_stop(struct mrpc_server *server);

#ifdef __cplusplus
}
#endif

#endif
