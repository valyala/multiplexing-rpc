#ifndef MRPC_METHOD_FACTORY_PUBLIC
#define MRPC_METHOD_FACTORY_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_method.h"
#include "mrpc/mrpc_data.h"
#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This callback shuld process rpc sent to the server by the client.
 * It can obtain request parameters from the data using the mrpc_method_get_request_param_value()
 * and set response parameters using the mrpc_method_set_response_param_value().
 * The service_ctx, which is passed to the mrpc_server_start(), usually used for persisting
 * service's state across rpc calls.
 * The callback is called by the mrpc_method_invoke_callback() function.
 */
typedef void (*mrpc_method_callback)(struct mrpc_data *data, void *service_ctx);

/**
 * Creates the server method.
 * request_param_constructors and response_param_constructors must contain NULL-terminated arrays
 * of corresponding constructors.
 * These parameters cannot be NULL.
 * The callback will be invoked for the given method by the rpc server. It cannot be NULL.
 * The returned method must be deleted using the mrpc_method_delete().
 * Always returns correct result.
 */
MRPC_API struct mrpc_method *mrpc_method_create_server_method(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, mrpc_method_callback callback);

/**
 * Creates the client method.
 * request_param_constructors and response_param_constructors must contain NULL-terminated arrays
 * of corresponding constructors.
 * These parameters cannot be NULL.
 * The is_key must contain an array of integers with the length equal to the number of constructors
 * in the request_param_constructors. It is used for determining whether the given parameter must be
 * used for calculating the hash of the given request.
 * It cannot be NULL.
 * The returned method must be deleted using the mrpc_method_delete().
 * Always returns correct result.
 */
MRPC_API struct mrpc_method *mrpc_method_create_client_method(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, int *is_key);

/**
 * Deletes the method, which could be created by the mrpc_method_create_server_method()
 * or by the mrpc_method_create_client_method().
 */
MRPC_API void mrpc_method_delete(struct mrpc_method *method);

#ifdef __cplusplus
}
#endif

#endif
