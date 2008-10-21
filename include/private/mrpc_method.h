#ifndef MRPC_METHOD_PRIVATE
#define MRPC_METHOD_PRIVATE

#include "mrpc/mrpc_method.h"
#include "private/mrpc_param.h"
#include "private/mrpc_data.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_param **mrpc_method_create_request_params(struct mrpc_method *method);

struct mrpc_param **mrpc_method_create_response_params(struct mrpc_method *method);

void mrpc_method_delete_request_params(struct mrpc_method *method, struct mrpc_param **request_params);

void mrpc_method_delete_response_params(struct mrpc_method *method, struct mrpc_param **response_params);

enum ff_result mrpc_method_read_request_params(struct mrpc_method *method, struct mrpc_param **request_params, struct ff_stream *stream);

enum ff_result mrpc_method_read_response_params(struct mrpc_method *method, struct mrpc_param **response_params, struct ff_stream *stream);

enum ff_result mrpc_method_write_request_params(struct mrpc_method *method, struct mrpc_param **request_params, struct ff_stream *stream);

enum ff_result mrpc_method_write_response_params(struct mrpc_method *method, struct mrpc_param **response_params, struct ff_stream *stream);

void mrpc_method_set_request_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **request_params, const void *value);

void mrpc_method_set_response_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **response_params, const void *value);

void mrpc_method_get_request_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **request_params, void **value);

void mrpc_method_get_response_param_value(struct mrpc_method *method, int param_idx, struct mrpc_param **response_params, void **value);

uint32_t mrpc_method_get_request_hash(struct mrpc_method *method, uint32_t start_value, struct mrpc_param **request_params);

void mrpc_method_invoke_callback(struct mrpc_method *method, struct mrpc_data *data, void *service_ctx);

#ifdef __cplusplus
}
#endif

#endif
