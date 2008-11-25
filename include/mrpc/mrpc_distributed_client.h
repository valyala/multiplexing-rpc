#ifndef MRPC_DISTRIBUTED_CLIENT_PUBLIC_H
#define MRPC_DISTRIBUTED_CLIENT_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_distributed_client_controller.h"
#include "mrpc/mrpc_client.h"
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
 * Starts the distributed_client using the given controller.
 * The controller must exist until the mrpc_distributed_client_stop() will be called.
 */
MRPC_API void mrpc_distributed_client_start(struct mrpc_distributed_client *distributed_client, struct mrpc_distributed_client_controller *controller);

/**
 * Stops the distributed_client. The caller must remove the controller, which has been passed
 * to the mrpc_distributed_client_start().
 */
MRPC_API void mrpc_distributed_client_stop(struct mrpc_distributed_client *distributed_client);

/**
 * Acquires a client for the given request_hash_value from the distributed_client.
 * this client must be released by the mrpc_distributed_client_release_client().
 * cookie is an opaque value, which must be passed to the mrpc_distributed_client_release_client().
 * Returns NULL if the client cannot be acquired, because the controller didn't added any clients to the distributed_client.
 */
MRPC_API struct mrpc_client *mrpc_distributed_client_acquire_client(struct mrpc_distributed_client *distributed_client, uint32_t request_hash_value, const void **cookie);

/**
 * Releases the client, which has been acquire using mrpc_distributed_client_acquire_client().
 * cookie is returned by the mrpc_distributed_client_acquire_client().
 */
MRPC_API void mrpc_distributed_client_release_client(struct mrpc_distributed_client *distributed_client, struct mrpc_client *client, const void *cookie);

#ifdef __cplusplus
}
#endif

#endif
