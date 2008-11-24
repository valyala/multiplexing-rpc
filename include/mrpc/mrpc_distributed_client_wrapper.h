#ifndef MRPC_DISTRIBUTED_CLIENT_WRAPPER_PUBLIC_H
#define MRPC_DISTRIBUTED_CLIENT_WRAPPER_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_client.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_distributed_client_wrapper;

/**
 * Returns the mrpc_client wrapped by the client_wrapper.
 * There is no need to call mrpc_client_delete() for the returned client,
 * because the client_wrapper is responsible for its deletion.
 */
MRPC_API struct mrpc_client *mrpc_distributed_client_wrapper_get_client(struct mrpc_distributed_client_wrapper *client_wrapper);

#ifdef __cplusplus
}
#endif

#endif
