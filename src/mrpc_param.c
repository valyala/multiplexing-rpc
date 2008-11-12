#include "private/mrpc_common.h"

#include "private/mrpc_param.h"

struct mrpc_param
{
	const struct mrpc_param_vtable *vtable;
	void *ctx;
};

struct mrpc_param *mrpc_param_create(const struct mrpc_param_vtable *vtable, void *ctx)
{
	struct mrpc_param *param;

	param = (struct mrpc_param *) ff_malloc(sizeof(*param));
	param->vtable = vtable;
	param->ctx = ctx;

	return param;
}

void mrpc_param_delete(struct mrpc_param *param)
{
	param->vtable->delete(param);
	ff_free(param);
}

void *mrpc_param_get_ctx(struct mrpc_param *param)
{
	return param->ctx;
}

enum ff_result mrpc_param_read_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	enum ff_result result;

	result = param->vtable->read_from_stream(param, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot read the param=%p from the stream=%p. See previous messages for more info", param, stream);
	}
	return result;
}

enum ff_result mrpc_param_write_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	enum ff_result result;

	result = param->vtable->write_to_stream(param, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write the param=%p to the stream=%p. See previous messages for more info", param, stream);
	}
	return result;
}

void mrpc_param_get_value(struct mrpc_param *param, void **value)
{
	param->vtable->get_value(param, value);
}

void mrpc_param_set_value(struct mrpc_param *param, const void *value)
{
	param->vtable->set_value(param, value);
}

uint32_t mrpc_param_get_hash(struct mrpc_param *param, uint32_t start_value)
{
	uint32_t hash_value;

	hash_value = param->vtable->get_hash(param, start_value);
	return hash_value;
}
