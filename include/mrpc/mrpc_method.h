#ifndef MRPC_METHOD_PUBLIC_H
#define MRPC_METHOD_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_method;

/* forward declaration of the mrpc_data structure.
 * we cannot include mrpc/mrpc_data.h here, because it will create circular dependency through mrpc/mrpc_interface.h
 */
struct mrpc_data;

/**
 * This callback should process rpc sent to the server by the client.
 * It can obtain request parameters from the data using the mrpc_method_get_request_param_value()
 * and set response parameters using the mrpc_method_set_response_param_value().
 * The service_ctx, which is passed to the mrpc_server_start(), usually used for persisting
 * service's state across rpc calls.
 */
typedef void (*mrpc_method_callback)(struct mrpc_data *data, void *service_ctx);

/**
 * this structure describes client method.
 * The request_param_constructors must contain NULL-terminated list of parameter constructors for request.
 * The response_param_constructors must contain NULL-terminated list of parameter constructors for response.
 * The is_key must contain an array of integers with the length equal to the number of constructors
 * in the request_param_constructors. It is used for determining whether the given parameter must be
 * used for calculating the hash of the given request. It cannot be NULL.
 */
struct mrpc_method_client_description
{
	const mrpc_param_constructor *request_param_constructors;
	const mrpc_param_constructor *response_param_constructors;
	const int *is_key;
};

/**
 * this structure describes server method.
 * The request_param_constructors must contain NULL-terminated list of parameter constructors for request.
 * The response_param_constructors must contain NULL-terminated list of parameter constructors for response.
 * The callback will be invoked for the given method by the rpc server. It cannot be NULL.
 */
struct mrpc_method_server_description
{
	const mrpc_param_constructor *request_param_constructors;
	const mrpc_param_constructor *response_param_constructors;
	const mrpc_method_callback callback;
};

#ifdef __cplusplus
}
#endif

#endif
