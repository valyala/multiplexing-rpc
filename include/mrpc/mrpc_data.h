#ifndef MRPC_DATA_PUBLIC_H
#define MRPC_DATA_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_data;

/**
 * Creates a mrpc_data for the given method_id from the interface.
 * method_id must be valid id of the method in the given interface.
 * Always returns correct result.
 */
MRPC_API struct mrpc_data *mrpc_data_create(struct mrpc_interface *interface, uint8_t method_id);

/**
 * Deletes the data.
 */
MRPC_API void mrpc_data_delete(struct mrpc_data *data);

/**
 * Returns the value of the request parameter with the param_idx index stored in the given data.
 * Usually this function must be called in the rpc server callback in order to obtain
 * request parameters returned from the client.
 */
MRPC_API void mrpc_data_get_request_param_value(struct mrpc_data *data, int param_idx, void **value);

/**
 * Returns the value of the response parameter with the param_id index stored in the given data.
 * Usually this function must be called by the rpc client in order to obtain
 * response parameters returned from tne server.
 */
MRPC_API void mrpc_data_get_response_param_value(struct mrpc_data *data, int param_idx, void **value);

/**
 * Stores the value in the request parameter with the param_id index stored in the given data.
 * Usually this function must be called in the rpc client in order to set request parameters before
 * sending request to the server.
 */
MRPC_API void mrpc_data_set_request_param_value(struct mrpc_data *data, int param_idx, const void *value);

/**
 * Stores the value in the response parameter with the param_id index stored in the given data.
 * Usually this function must be called in the rpc server callback in order to set response parameters before
 * sending response to the client.
 */
MRPC_API void mrpc_data_set_response_param_value(struct mrpc_data *data, int param_idx, const void *value);

/**
 * Returns the hash value for the given start_value and request parameters stored in the given data.
 * Usually this function must be called in the rpc client in order to determine to which server
 * the given request should be sent (distributed setup).
 */
MRPC_API uint32_t mrpc_data_get_request_hash(struct mrpc_data *data, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
