#ifndef MRPC_CLIENT_PUBLIC_H
#define MRPC_CLIENT_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_data.h"
#include "mrpc/mrpc_interface.h"
#include "ff/ff_stream_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_client;

/**
 * Creates an mrpc client.
 * Always returns correct result.
 */
MRPC_API struct mrpc_client *mrpc_client_create();

/**
 * Deletes the client.
 */
MRPC_API void mrpc_client_delete(struct mrpc_client *client);

/**
 * Starts the given client, which will use the given client_interface for rpc.
 * The client will use the stream_connector in order to establish connection with mrpc server.
 * The client is able to automatically reconnect to the mrpc server on errors.
 * This function returns immediately.
 */
MRPC_API void mrpc_client_start(struct mrpc_client *client, const struct mrpc_interface *client_interface, struct ff_stream_connector *stream_connector);

/**
 * Stops the given client.
 * After the client is stopped, it can be started again using the mrpc_client_start() function.
 * This function returns olny when the client is stopped.
 */
MRPC_API void mrpc_client_stop(struct mrpc_client *client);

/**
 * creates data for the method with the given method_id, which must be valid for the interface,
 * passed to the mrpc_client_start() method before.
 * The returned data must be deleted by the caller using the mrpc_data_delete() method.
 * This function can be called only after the mrpc_client_start() has been called.
 * Always returns correct result.
 */
MRPC_API struct mrpc_data *mrpc_client_create_data(struct mrpc_client *client, uint8_t method_id);

/**
 * Invokes the given rpc data on the client.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_client_invoke_rpc(struct mrpc_client *client, struct mrpc_data *data);

#ifdef __cplusplus
}
#endif

#endif
