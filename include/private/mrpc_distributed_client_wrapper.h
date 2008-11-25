#ifndef MRPC_DISTRIBUTED_CLIENT_WRAPPER_PRIVATE_H
#define MRPC_DISTRIBUTED_CLIENT_WRAPPER_PRIVATE_H

#include "ff/ff_stream_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a client_wrapper.
 */
struct mrpc_distributed_client_wrapper *mrpc_distributed_client_wrapper_create();

/**
 * Deletes the client_wrapper.
 */
void mrpc_distributed_client_wrapper_delete(struct mrpc_distributed_client_wrapper *client_wrapper);

/**
 * Starts the client, which will use the given stream_connector.
 * The client_wrapper acquires ownership of the stream_connector, so there is no need to delete it.
 */
void mrpc_distributed_client_wrapper_start(struct mrpc_distributed_client_wrapper *client_wrapper, struct ff_stream_connector *stream_connector);

/**
 * Stops the client_wrapper.
 * It waits while all client references will released using mrpc_distributed_client_wrapper_release_client().
 */
void mrpc_distributed_client_wrapper_stop(struct mrpc_distributed_client_wrapper *client_wrapper);

/**
 * Acquires the mrpc_client wrapped by the client_wrapper.
 * The returned client must be released using the mrpc_distributed_client_wrapper_release_client() call.
 */
struct mrpc_client *mrpc_distributed_client_wrapper_acquire_client(struct mrpc_distributed_client_wrapper *client_wrapper);

/**
 * Releases the client, which has been acquired using mrpc_distributed_client_wrapper_acquire_client().
 */
void mrpc_distributed_client_wrapper_release_client(struct mrpc_distributed_client_wrapper *client_wrapper, struct mrpc_client *client);


#ifdef __cplusplus
}
#endif

#endif
