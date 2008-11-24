#ifndef MRPC_DISTRIBUTED_CLIENT_WRAPPER_PRIVATE_H
#define MRPC_DISTRIBUTED_CLIENT_WRAPPER_PRIVATE_H

#include "mrpc/mrpc_distributed_client_wrapper.h"
#include "ff/ff_stream_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_distributed_client_wrapper *mrpc_distributed_client_wrapper_create();

void mrpc_distributed_client_wrapper_inc_ref(struct mrpc_distributed_client_wrapper *client_wrapper);

void mrpc_distributed_client_wrapper_dec_ref(struct mrpc_distributed_client_wrapper *client_wrapper);

void mrpc_distributed_client_wrapper_start(struct mrpc_distributed_client_wrapper *client_wrapper, struct ff_stream_connector *stream_connector);

void mrpc_distributed_client_wrapper_stop(struct mrpc_distributed_client_wrapper *client_wrapper);

#ifdef __cplusplus
}
#endif

#endif
