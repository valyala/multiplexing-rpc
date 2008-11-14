#include "private/mrpc_common.h"

#include "private/mrpc_data.h"
#include "private/mrpc_method.h"
#include "private/mrpc_param.h"
#include "private/mrpc_interface.h"
#include "ff/ff_stream.h"

struct mrpc_data
{
	struct mrpc_method *method;
	struct mrpc_param **request_params;
	struct mrpc_param **response_params;
};

static struct mrpc_data *try_create_mrpc_data(struct mrpc_interface *interface, uint8_t method_id)
{
	struct mrpc_method *method;
	struct mrpc_data *data = NULL;

	method = mrpc_interface_get_method(interface, method_id);
	if (method != NULL)
	{
		data = (struct mrpc_data *) ff_malloc(sizeof(*data));
		data->method = method;
		data->request_params = mrpc_method_create_request_params(method);
		data->response_params = mrpc_method_create_response_params(method);
	}
	else
	{
		ff_log_debug(L"the interface=%p doesn't contain method with method_id=%lu", interface, (uint32_t) method_id);
	}

	return data;
}

static struct mrpc_data *read_request(struct mrpc_interface *interface, struct ff_stream *stream)
{
	uint8_t method_id;
	struct mrpc_data *data = NULL;
	enum ff_result result;

	result = ff_stream_read(stream, &method_id, 1);
	if (result == FF_SUCCESS)
	{
		data = try_create_mrpc_data(interface, method_id);
		if (data != NULL)
		{
			int request_params_cnt;

			request_params_cnt = mrpc_method_get_request_params_cnt(data->method);
			if (request_params_cnt > 0)
			{
				result = mrpc_method_read_request_params(data->method, data->request_params, stream);
				if (result != FF_SUCCESS)
				{
					ff_log_debug(L"cannot read request paramters for the method=%p with method_id=%lu from the stream=%p. See previous messages for more info",
						data->method, (uint32_t) method_id, stream);
					mrpc_data_delete(data);
					data = NULL;
				}
			}
		}
		else
		{
			ff_log_debug(L"wrong method_id=%lu has been read from the stream=%p for the interface=%p. See previous messages for more info",
				(uint32_t) method_id, stream, interface);
		}
	}
	else
	{
		ff_log_debug(L"cannot read method_id from the stream=%p for the interface=%p. See previous messages for more info", stream, interface);
	}
	return data;
}

static enum ff_result read_response(struct mrpc_data *data, struct ff_stream *stream)
{
	int response_params_cnt;
	enum ff_result result;

	response_params_cnt = mrpc_method_get_response_params_cnt(data->method);
	if (response_params_cnt == 0)
	{
		/* read an empty byte from the stream in order to determine the moment,
		 * when the response will be received.
		 */
		uint8_t empty_byte;

		result = ff_stream_read(stream, &empty_byte, 1);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot read an empty byte from the stream=%p for the method=%p. See previous messages for more info", stream, data->method);
		}
		if (empty_byte != 0)
		{
			ff_log_debug(L"wrong empty_byte=%d read from the stream=%p for the method=%p. Expected value is 0", empty_byte, stream, data->method);
			result = FF_FAILURE;
		}
	}
	else
	{
		result = mrpc_method_read_response_params(data->method, data->response_params, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot read response paramters for the method=%p from the stream=%p. See previous messages for more info", data->method, stream);
		}
	}

	return result;
}

static enum ff_result write_response(struct mrpc_data *data, struct ff_stream *stream)
{
	int response_params_cnt;
	enum ff_result result;

	response_params_cnt = mrpc_method_get_response_params_cnt(data->method);
	if (response_params_cnt == 0)
	{
		/* write an empty byte to the stream in order to signal the receiver about response arrival
		 */
		uint8_t empty_byte;

		empty_byte = 0;
		result = ff_stream_write(stream, &empty_byte, 1);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot write an empty byte to the stream=%p for the method=%p. See previous messages for more info", stream, data->method);
		}
	}
	else
	{
		result = mrpc_method_write_response_params(data->method, data->response_params, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot write response parameters for the method=%p to the stream=%p. See previous messages for more info", data->method, stream);
		}
	}

	return result;
}

static enum ff_result write_request(struct mrpc_data *data, struct ff_stream *stream)
{
	enum ff_result result;
	uint8_t method_id;

	method_id = mrpc_method_get_method_id(data->method);
	result = ff_stream_write(stream, &method_id, 1);
	if (result == FF_SUCCESS)
	{
		int request_params_cnt;

		request_params_cnt = mrpc_method_get_request_params_cnt(data->method);
		if (request_params_cnt > 0)
		{
			result = mrpc_method_write_request_params(data->method, data->request_params, stream);
			if (result != FF_SUCCESS)
			{
				ff_log_debug(L"cannot write request paramters for the method=%p to the stream=%p. See previous messages for more info", data->method, stream);
			}
		}
	}
	else
	{
		ff_log_debug(L"cannot write method_id=%lu to the stream=%p. See previous messages for more info", (uint32_t) method_id, stream);
	}
	return result;
}

struct mrpc_data *mrpc_data_create(struct mrpc_interface *interface, uint8_t method_id)
{
	struct mrpc_data *data;

	data = try_create_mrpc_data(interface, method_id);
	/* it is expected that the method_id, which was passed to the mrpc_data_create(), is valid,
	 * so the try_create_mrpc_data() must return non-NULL data.
	 */
	ff_assert(data != NULL);
	return data;
}

void mrpc_data_delete(struct mrpc_data *data)
{
	mrpc_method_delete_request_params(data->method, data->request_params);
	mrpc_method_delete_response_params(data->method, data->response_params);
	ff_free(data);
}

enum ff_result mrpc_data_process_remote_call(struct mrpc_interface *interface, void *service_ctx, struct ff_stream *stream)
{
	struct mrpc_data *data;
	enum ff_result result = FF_FAILURE;

	data = read_request(interface, stream);
	if (data != NULL)
	{
		mrpc_method_invoke_callback(data->method, data, service_ctx);
		result = write_response(data, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot write response for data=%p to the stream=%p. See previous messages for more info", data, stream);
		}

		/* stream flushing must be performed after data will be deleted,
		 * because mrpc_data_delete() can block, so the response will be sent to the client, which, in turn
		 * can issue yet another request to the server with the request_id, which is occupied by the current
		 * request processor. So the server won't process this request.
		 */
		mrpc_data_delete(data);
		if (result == FF_SUCCESS)
		{
			/* it is assumed that the ff_stream_flush() won't block, otherwise the client can send yet anohter
			 * request with the same request_id while the current request processor will be blocked here.
			 */
			result = ff_stream_flush(stream);
			if (result != FF_SUCCESS)
			{
				ff_log_debug(L"cannot flush the stream=%p after writing response for the method=%p. See previous messages for more info", stream, data->method);
			}
		}
    }
    else
    {
    	ff_log_debug(L"cannot read request from the stream=%p for the interface=%p. See previous messages for more info", stream, interface);
    }

    return result;
}

enum ff_result mrpc_data_invoke_remote_call(struct mrpc_data *data, struct ff_stream *stream)
{
	enum ff_result result;

	result = write_request(data, stream);
	if (result == FF_SUCCESS)
	{
		result = ff_stream_flush(stream);
		if (result == FF_SUCCESS)
		{
			result = read_response(data, stream);
			if (result != FF_SUCCESS)
			{
				ff_log_debug(L"cannot read response for data=%p from the stream=%p. See previous messages for more info", data, stream);
			}
		}
		else
		{
			ff_log_debug(L"cannot flush the stream=%p after writing request for the method=%p. See prevois messages for more info", stream, data->method);
		}
	}
	else
	{
		ff_log_debug(L"cannot write request for data=%p to the stream=%p. See previous messages for more info", data, stream);
	}

	return result;
}

void mrpc_data_get_request_param_value(struct mrpc_data *data, int param_idx, void **value)
{
	mrpc_method_get_request_param_value(data->method, param_idx, data->request_params, value);
}

void mrpc_data_get_response_param_value(struct mrpc_data *data, int param_idx, void **value)
{
	mrpc_method_get_response_param_value(data->method, param_idx, data->response_params, value);
}

void mrpc_data_set_request_param_value(struct mrpc_data *data, int param_idx, const void *value)
{
	mrpc_method_set_request_param_value(data->method, param_idx, data->request_params, value);
}

void mrpc_data_set_response_param_value(struct mrpc_data *data, int param_idx, const void *value)
{
	mrpc_method_set_response_param_value(data->method, param_idx, data->response_params, value);
}

uint32_t mrpc_data_get_request_hash(struct mrpc_data *data, uint32_t start_value)
{
	uint32_t hash_value;

	hash_value = mrpc_method_get_request_hash(data->method, start_value, data->request_params);
	return hash_value;
}
