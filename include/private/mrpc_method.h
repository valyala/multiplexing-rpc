#ifndef MRPC_METHOD_PRIVATE
#define MRPC_METHOD_PRIVATE

#include "mrpc/mrpc_method.h"
#include "private/mrpc_param.h"
#include "private/mrpc_data.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates the server method.
 * request_param_constructors and response_param_constructors must contain NULL-terminated arrays
 * of corresponding constructors. These parameters cannot be NULL.
 * The callback will be invoked for the given method by the rpc server. It cannot be NULL.
 * The returned method must be deleted using the mrpc_method_delete().
 * Always returns correct result.
 */
struct mrpc_method *mrpc_method_create_server_method(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, mrpc_method_callback callback, uint8_t method_id);

/**
 * Creates the client method.
 * request_param_constructors and response_param_constructors must contain NULL-terminated arrays
 * of corresponding constructors. These parameters cannot be NULL.
 * The is_key must contain an array of integers with the length equal to the number of constructors
 * in the request_param_constructors. It is used for determining whether the given parameter must be
 * used for calculating the hash of the given request. It cannot be NULL.
 * The returned method must be deleted using the mrpc_method_delete().
 * Always returns correct result.
 */
struct mrpc_method *mrpc_method_create_client_method(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, const int *is_key, uint8_t method_id);

/**
 * Deletes the method, which could be created by the mrpc_method_create_server_method()
 * or by the mrpc_method_create_client_method().
 */
void mrpc_method_delete(struct mrpc_method *method);

/**
 * creates request parameters for the given method.
 * These parameters should be deleted by the caller using the mrpc_method_delete_request_params().
 */
struct mrpc_param **mrpc_method_create_request_params(struct mrpc_method *method);

/**
 * creates response parameters for the given method.
 * These parameters should be deleted by the caller using the mrpc_method_delete_response_params().
 */
struct mrpc_param **mrpc_method_create_response_params(struct mrpc_method *method);

/**
 * deletes request parameters, which was created using the mrpc_method_create_request_params().
 */
void mrpc_method_delete_request_params(struct mrpc_method *method, struct mrpc_param **request_params);

/**
 * deletes response parameters, which was created using the mrpc_method_create_response_params().
 */
void mrpc_method_delete_response_params(struct mrpc_method *method, struct mrpc_param **response_params);

/**
 * reads request parameters for the given method from the given stream.
 * This function must be called by the rpc server in order to obtain method's request parameters sent by client.
 * This function can be called only if the mrpc_method_get_request_params_cnt() returns non-zero value.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_method_read_request_params(struct mrpc_method *method, struct mrpc_param **request_params, struct ff_stream *stream);

/**
 * reads response parameters for the given method from the given stream.
 * This function must be called be the rpc client in order to obtain method's response parameters sent by server.
 * This function can be called only if the mrpc_method_get_response_params_cnt() returns non-zero value.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_method_read_response_params(struct mrpc_method *method, struct mrpc_param **response_params, struct ff_stream *stream);

/**
 * writes request parameters for the given method to the given stream.
 * This function must be called by the rpc client in order to send method's request parameters to the server.
 * This function can be called only if the mrpc_method_get_request_params_cnt() returns non-zero value.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_method_write_request_params(struct mrpc_method *method, struct mrpc_param **request_params, struct ff_stream *stream);

/**
 * writes response parameters for the given method to the given stream.
 * This function must be called by the rpc server in order to send method's response parameters to the client.
 * This function can be called only if the mrpc_method_get_response_params_cnt() returns non-zero value.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_method_write_response_params(struct mrpc_method *method, struct mrpc_param **response_params, struct ff_stream *stream);

/**
 * Sets the value of the request parameter with param_idx index from the given method to the given value.
 * This function must be called by the rpc client before sending request to the server.
 */
void mrpc_method_set_request_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **request_params, const void *value);

/**
 * Sets the value of the response parameter with param_idx index from the given method to the given value.
 * This function must be called by the rpc server before sending response to the client.
 */
void mrpc_method_set_response_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **response_params, const void *value);

/**
 * Obtains the value of the request parameter with param_idx index from the given method.
 * This function must be called by the rpc server after retreiving request from the client.
 */
void mrpc_method_get_request_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **request_params, void **value);

/**
 * Obtains the value of the response parameter with param_idx index from the given method.
 * This function must be called by the rpc client after retreiving response from the server.
 */
void mrpc_method_get_response_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **response_params, void **value);

/**
 * Calculates hash value for request parameters of the given method and for the given start_value.
 * This function should be called by the rpc client after setting up all the request parameters in order
 * to determine to which client the given request should be sent. This is usually used in distributed setup.
 */
uint32_t mrpc_method_get_request_hash(struct mrpc_method *method, uint32_t start_value, struct mrpc_param **request_params);

/**
 * Invokes the method's callback with the given data and service_ctx.
 * This function must be called by the rpc server after receiving all request parameters for the given method
 * from the client.
 * The callback is passed to the mrpc_method_create_server_method() function.
 */
void mrpc_method_invoke_callback(struct mrpc_method *method, struct mrpc_data *data, void *service_ctx);

/**
 * returns the number of request paramters for the given method
 */
int mrpc_method_get_request_params_cnt(struct mrpc_method *method);

/**
 * returns the number of response paramters for the given method
 */
int mrpc_method_get_response_params_cnt(struct mrpc_method *method);

/**
 * returns method_id, which was passed to the mrpc_method_create_{client|server}_method()
 */
uint8_t mrpc_method_get_method_id(struct mrpc_method *method);

#ifdef __cplusplus
}
#endif

#endif
