#ifndef MRPC_DISTRIBUTED_CLIENT_PUBLIC_H
#define MRPC_DISTRIBUTED_CLIENT_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_distributed_client_wrapper.h"
#include "ff/ff_stream_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_distributed_client;

/**
 * Creates the distributed client.
 * Always returns correct result.
 */
MRPC_API struct mrpc_distributed_client *mrpc_distributed_client_create();

/**
 * Deletes the distributed client.
 */
MRPC_API void mrpc_distributed_client_delete(struct mrpc_distributed_client *distributed_client);

/**
 * Removes all the clients added to the distributed client.
 */
MRPC_API void mrpc_distributed_client_remove_all_clients(struct mrpc_distributed_client *distributed_client);

/**
 * Starts new client using the given stream_connector. The client is associated with the given key.
 */
MRPC_API void mrpc_distributed_client_add_client(struct mrpc_distributed_client *distributed_client, struct ff_stream_connector *stream_connector, uint64_t key);

/**
 * Removes the client with the given key from the distributed_client.
 */
MRPC_API void mrpc_distributed_client_remove_client(struct mrpc_distributed_client *distributed_client, uint64_t key);

/**
 * Acquires a client for the given request_hash_value from the distributed_client.
 * this client must be released by the mrpc_distributed_client_release_client().
 * Returns NULL if the client cannot be acquired, because the controller didn't added any clients to the distributed_client.
 */
MRPC_API struct mrpc_distributed_client_wrapper *mrpc_distributed_client_acquire_client(struct mrpc_distributed_client *distributed_client, uint32_t request_hash_value);

/**
 * Releases the client, which has been acquire using mrpc_distributed_client_acquire_client().
 */
MRPC_API void mrpc_distributed_client_release_client(struct mrpc_distributed_client *distributed_client, struct mrpc_distributed_client_wrapper *client_wrapper);

#ifdef __cplusplus
}
#endif

#endif