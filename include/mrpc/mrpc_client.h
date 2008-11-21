#ifndef MRPC_CLIENT_PUBLIC_H
#define MRPC_CLIENT_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream_connector.h"
#include "ff/ff_stream.h"

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
 * Starts the given client.
 * The client will use the stream_connector in order to establish connection with mrpc server.
 * The client is able to automatically reconnect to the mrpc server on errors.
 * This function returns immediately.
 */
MRPC_API void mrpc_client_start(struct mrpc_client *client, struct ff_stream_connector *stream_connector);

/**
 * Stops the given client.
 * After the client is stopped, it can be started again using the mrpc_client_start() function.
 * This function returns olny when the client is stopped.
 */
MRPC_API void mrpc_client_stop(struct mrpc_client *client);

/**
 * creates request stream for sending rpc request.
 * This request stream must be deleted using the ff_stream_delete().
 * Returns request stream on success, NULL if the request stream cannot be created during the tries_cnt tries.
 */
MRPC_API struct ff_stream *mrpc_client_create_request_stream(struct mrpc_client *client, int tries_cnt);

/**
 * closes the underlying connection to the server and opens new one.
 * Use this method if the stream returned from the mrpc_client_create_request_stream()
 * returns garbage (i.e. broken client-server protocol synchronization).
 */
MRPC_API void mrpc_client_reset_connection(struct mrpc_client *client);

#ifdef __cplusplus
}
#endif

#endif
