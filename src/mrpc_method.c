#include "private/mrpc_common.h"

#include "private/mrpc_method.h"
#include "private/mrpc_param.h"
#include "private/mrpc_data.h"
#include "ff/ff_stream.h"

/**
 * Maximum number of parameters in the request or response of the rpc.
 * This value has no practical limitations except 0 < MAX_PARAMS_CNT < INT_MAX.
 */
#define MAX_PARAMS_CNT 100

struct mrpc_method
{
	const mrpc_param_constructor *request_param_constructors;
	const mrpc_param_constructor *response_param_constructors;
	mrpc_method_callback callback;
	const int *is_key;
	int request_params_cnt;
	int response_params_cnt;
	uint8_t method_id;
};

static int get_constructors_cnt(const mrpc_param_constructor *param_constructors)
{
	int constructors_cnt;
	mrpc_param_constructor param_constructor;

	constructors_cnt = -1;
	do
	{
		constructors_cnt++;
		param_constructor = param_constructors[constructors_cnt];
	} while (constructors_cnt <= MAX_PARAMS_CNT && param_constructor != NULL);
	ff_assert(constructors_cnt <= MAX_PARAMS_CNT);

	return constructors_cnt;
}

static struct mrpc_param **create_params(const mrpc_param_constructor *param_constructors, int params_cnt)
{
	int i;
	struct mrpc_param **params;

	ff_assert(params_cnt >= 0);
	ff_assert(params_cnt <= MAX_PARAMS_CNT);
	params = (struct mrpc_param **) ff_calloc(params_cnt, sizeof(params[0]));
	for (i = 0; i < params_cnt; i++)
	{
		mrpc_param_constructor param_constructor;
		struct mrpc_param *param;

		param_constructor = param_constructors[i];
		ff_assert(param_constructor != NULL);
		param = param_constructor();
		ff_assert(param != NULL);
		params[i] = param;
	}

	return params;
}

static void delete_params(struct mrpc_param **params, int param_cnt)
{
	int i;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt <= MAX_PARAMS_CNT);
	for (i = 0; i < param_cnt; i++)
	{
		struct mrpc_param *param;

		param = params[i];
		ff_assert(param != NULL);
		mrpc_param_delete(param);
	}
	ff_free(params);
}

static enum ff_result read_params(struct mrpc_param **params, int param_cnt, struct ff_stream *stream)
{
	int i;
	enum ff_result result = FF_SUCCESS;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt <= MAX_PARAMS_CNT);
	for (i = 0; i < param_cnt; i++)
	{
		struct mrpc_param *param;

		param = params[i];
		ff_assert(param != NULL);
		result = mrpc_param_read_from_stream(param, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot read parameter number %d from the stream=%p. See previous messages for more info", i, stream);
			break;
		}
	}

	return result;
}

static enum ff_result write_params(struct mrpc_param **params, int param_cnt, struct ff_stream *stream)
{
	int i;
	enum ff_result result = FF_SUCCESS;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt <= MAX_PARAMS_CNT);
	for (i = 0; i < param_cnt; i++)
	{
		struct mrpc_param *param;

		param = params[i];
		result = mrpc_param_write_to_stream(param, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot write paramter number %d to the stream=%p. See previous messages for more info", i, stream);
			break;
		}
	}

	return result;
}

static void get_param_value(struct mrpc_param **params, void **value, int param_idx, int params_cnt)
{
	struct mrpc_param *param;

	ff_assert(params_cnt >= 0);
	ff_assert(params_cnt <= MAX_PARAMS_CNT);
	ff_assert(param_idx >= 0);
	ff_assert(param_idx < params_cnt);

	param = params[param_idx];
	mrpc_param_get_value(param, value);
}

static void set_param_value(struct mrpc_param **params, const void *value, int param_idx, int params_cnt)
{
	struct mrpc_param *param;

	ff_assert(params_cnt >= 0);
	ff_assert(params_cnt <= MAX_PARAMS_CNT);
	ff_assert(param_idx >= 0);
	ff_assert(param_idx < params_cnt);

	param = params[param_idx];
	mrpc_param_set_value(param, value);
}

const struct mrpc_method *mrpc_method_create_server_method(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, mrpc_method_callback callback, uint8_t method_id)
{
	struct mrpc_method *method;

	ff_assert(request_param_constructors != NULL);
	ff_assert(response_param_constructors != NULL);
	ff_assert(callback != NULL);

	method = (struct mrpc_method *) ff_malloc(sizeof(*method));
	method->request_param_constructors = request_param_constructors;
	method->response_param_constructors = response_param_constructors;
	method->callback = callback;
	method->is_key = NULL;
	method->request_params_cnt = get_constructors_cnt(request_param_constructors);
	method->response_params_cnt = get_constructors_cnt(response_param_constructors);
	method->method_id = method_id;

	return method;
}

const struct mrpc_method *mrpc_method_create_client_method(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, const int *is_key, uint8_t method_id)
{
	struct mrpc_method *method;

	ff_assert(request_param_constructors != NULL);
	ff_assert(response_param_constructors != NULL);
	ff_assert(is_key != NULL);

	method = (struct mrpc_method *) ff_malloc(sizeof(*method));
	method->request_param_constructors = request_param_constructors;
	method->response_param_constructors = response_param_constructors;
	method->callback = NULL;
	method->is_key = is_key;
	method->request_params_cnt = get_constructors_cnt(request_param_constructors);
	method->response_params_cnt = get_constructors_cnt(response_param_constructors);
	method->method_id = method_id;

	return method;
}

void mrpc_method_delete(const struct mrpc_method *method)
{
	ff_free((void *) method);
}

struct mrpc_param **mrpc_method_create_request_params(const struct mrpc_method *method)
{
	struct mrpc_param **request_params;

	request_params = create_params(method->request_param_constructors, method->request_params_cnt);
	return request_params;
}

struct mrpc_param **mrpc_method_create_response_params(const struct mrpc_method *method)
{
	struct mrpc_param **response_params;

	response_params = create_params(method->response_param_constructors, method->response_params_cnt);
	return response_params;
}

void mrpc_method_delete_request_params(const struct mrpc_method *method, struct mrpc_param **request_params)
{
	delete_params(request_params, method->request_params_cnt);
}

void mrpc_method_delete_response_params(const struct mrpc_method *method, struct mrpc_param **response_params)
{
	delete_params(response_params, method->response_params_cnt);
}

enum ff_result mrpc_method_read_request_params(const struct mrpc_method *method, struct mrpc_param **request_params, struct ff_stream *stream)
{
	enum ff_result result;

	ff_assert(method->callback != NULL);
	ff_assert(method->is_key == NULL);
	ff_assert(method->request_params_cnt > 0);

	result = read_params(request_params, method->request_params_cnt, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot read %d request parameters for the method=%p from the stream=%p. See previous messages for more info", method->request_params_cnt, method, stream);
	}
	return result;
}

enum ff_result mrpc_method_read_response_params(const struct mrpc_method *method, struct mrpc_param **response_params, struct ff_stream *stream)
{
	enum ff_result result;

	ff_assert(method->callback == NULL);
	ff_assert(method->is_key != NULL);
	ff_assert(method->response_params_cnt > 0);

	result = read_params(response_params, method->response_params_cnt, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot read %d response paramters for the method=%p from the stream=%p. See previous messages for more info", method->response_params_cnt, method, stream);
	}
	return result;
}

enum ff_result mrpc_method_write_request_params(const struct mrpc_method *method, struct mrpc_param **request_params, struct ff_stream *stream)
{
	enum ff_result result;

	ff_assert(method->callback == NULL);
	ff_assert(method->is_key != NULL);
	ff_assert(method->request_params_cnt > 0);

	result = write_params(request_params, method->request_params_cnt, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write %d request paramters for the method=%p to the stream=%p. See previous messages for more info", method->request_params_cnt, method, stream);
	}
	return result;
}

enum ff_result mrpc_method_write_response_params(const struct mrpc_method *method, struct mrpc_param **response_params, struct ff_stream *stream)
{
	enum ff_result result;

	ff_assert(method->callback != NULL);
	ff_assert(method->is_key == NULL);
	ff_assert(method->response_params_cnt > 0);

	result = write_params(response_params, method->response_params_cnt, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write %d response paramters for the method=%d to the stream=%p. See previouse messages for more info", method->response_params_cnt, method, stream);
	}
	return result;
}

void mrpc_method_set_request_param_value(const struct mrpc_method *method, int param_idx, struct mrpc_param **request_params, const void *value)
{
	ff_assert(method->callback == NULL);
	ff_assert(method->is_key != NULL);

	set_param_value(request_params, value, param_idx, method->request_params_cnt);
}

void mrpc_method_set_response_param_value(const struct mrpc_method *method, int param_idx, struct mrpc_param **response_params, const void *value)
{
	ff_assert(method->callback != NULL);
	ff_assert(method->is_key == NULL);

	set_param_value(response_params, value, param_idx, method->response_params_cnt);
}

void mrpc_method_get_request_param_value(const struct mrpc_method *method, int param_idx, struct mrpc_param **request_params, void **value)
{
	ff_assert(method->callback != NULL);
	ff_assert(method->is_key == NULL);

	get_param_value(request_params, value, param_idx, method->request_params_cnt);
}

void mrpc_method_get_response_param_value(const struct mrpc_method *method, int param_idx, struct mrpc_param **response_params, void **value)
{
	ff_assert(method->callback == NULL);
	ff_assert(method->is_key != NULL);

	get_param_value(response_params, value, param_idx, method->response_params_cnt);
}

uint32_t mrpc_method_get_request_hash(const struct mrpc_method *method, uint32_t start_value, struct mrpc_param **request_params)
{
	const int *is_keys;
	int params_cnt;
	int i;
	uint32_t hash_value;

	ff_assert(method->callback == NULL);
	ff_assert(method->is_key != NULL);

	hash_value = start_value;
	params_cnt = method->request_params_cnt;
	is_keys = method->is_key;
	for (i = 0; i < params_cnt; i++)
	{
		int is_key;

		is_key = is_keys[i];
		if (is_key)
		{
			struct mrpc_param *param;

			param = request_params[i];
			hash_value = mrpc_param_get_hash(param, hash_value);
		}
	}

	return hash_value;
}

void mrpc_method_invoke_callback(const struct mrpc_method *method, struct mrpc_data *data, void *service_ctx)
{
	ff_assert(method->callback != NULL);
	ff_assert(method->is_key == NULL);

	method->callback(data, service_ctx);
}

int mrpc_method_get_request_params_cnt(const struct mrpc_method *method)
{
	ff_assert(method->request_params_cnt >= 0);

	return method->request_params_cnt;
}

int mrpc_method_get_response_params_cnt(const struct mrpc_method *method)
{
	ff_assert(method->response_params_cnt >= 0);

	return method->response_params_cnt;
}

uint8_t mrpc_method_get_method_id(const struct mrpc_method *method)
{
	return method->method_id;
}
