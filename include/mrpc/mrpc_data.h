#ifndef MRPC_DATA_PUBLIC
#define MRPC_DATA_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_data;

MRPC_API struct mrpc_data *mrpc_data_create(struct mrpc_interface *interface, uint8_t method_id);

MRPC_API void mrpc_data_delete(struct mrpc_data *data);

MRPC_API void mrpc_data_get_request_param_value(struct mrpc_data *data, int param_idx, void **value);

MRPC_API void mrpc_data_get_response_param_value(struct mrpc_data *data, int param_idx, void **value);

MRPC_API void mrpc_data_set_request_param_value(struct mrpc_data *data, int param_idx, const void *value);

MRPC_API void mrpc_data_set_response_param_value(struct mrpc_data *data, int param_idx, const void *value);

MRPC_API uint32_t mrpc_data_get_request_hash(struct mrpc_data *data, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
