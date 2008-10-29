#ifndef MRPC_CLIENT_PUBLIC
#define MRPC_CLIENT_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_data.h"
#include "ff/ff_stream_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_client;

MRPC_API struct mrpc_client *mrpc_client_create();

MRPC_API void mrpc_client_delete(struct mrpc_client *client);

MRPC_API void mrpc_client_start(struct mrpc_client *client, struct ff_stream_connector *stream_connector);

MRPC_API void mrpc_client_stop(struct mrpc_client *client);

MRPC_API enum ff_result mrpc_client_invoke_rpc(struct mrpc_client *client, struct mrpc_data *data);

#ifdef __cplusplus
}
#endif

#endif
