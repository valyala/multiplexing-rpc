#include "private/mrpc_common.h"

#include "private/mrpc_string_param.h"
#include "private/mrpc_param_vtable.h"
#include "private/mrpc_string_serialization.h"
#include "ff/ff_string.h"
#include "ff/ff_stream.h"

struct string_param
{
	struct ff_string *string;
};

static void *create_string_param()
{
	struct string_param *param;

	param = (struct string_param *) ff_malloc(sizeof(*param));
	param->string = NULL;
	return param;
}

static void delete_string_param(void *ctx)
{
	struct string_param *param;

	param = (struct string_param *) ctx;
	if (param->string != NULL)
	{
		ff_string_dec_ref(param->string);
	}
	ff_free(param);
}

static int read_string_param_from_stream(void *ctx, struct ff_stream *stream)
{
	struct string_param *param;
	int is_success;

	param = (struct string_param *) ctx;
	ff_assert(param->string == NULL);
	is_success = mrpc_string_unserialize(&param->string, stream);
	if (is_success)
	{
		ff_assert(param->string != NULL);
	}
	return is_success;
}

static int write_string_param_to_stream(const void *ctx, struct ff_stream *stream)
{
	struct string_param *param;
	int is_success;

	param = (struct string_param *) ctx;
	ff_assert(param->string != NULL);
	is_success = mrpc_string_serialize(param->string, stream);
	return is_success;
}

static void get_string_param_value(const void *ctx, void **value)
{
	struct string_param *param;

	param = (struct string_param *) ctx;
	ff_assert(param->string != NULL);
	*(struct ff_string **) value = param->string;
}

static void set_string_param_value(void *ctx, const void *value)
{
	struct string_param *param;

	param = (struct string_param *) ctx;
	ff_assert(param->string == NULL);
	param->string = (struct ff_string *) value;
}

static uint32_t get_string_param_hash(const void *ctx, uint32_t start_value)
{
	struct string_param *param;
	const wchar_t *cstr;
	uint32_t *tmp;
	int len;
	uint32_t hash;

	param = (struct string_param *) ctx;
	ff_assert(param->string != NULL);
	cstr = ff_string_get_cstr(param->string);
	len = ff_string_get_length(param->string);
	tmp = (uint32_t *) ff_malloc(len * sizeof(tmp[0]));
	for (i = 0; i < len; i++)
	{
		tmp[i] = cstr[i];
	}
	hash = ff_hash_uint32(start_value, tmp, len);
	ff_free(tmp);
	return hash;
}

static const struct mrpc_param_vtable string_param_vtable =
{
	create_string_param,
	delete_string_param,
	read_string_param_from_stream,
	write_string_param_to_stream,
	get_string_param_value,
	set_string_param_value,
	get_string_param_hash
};

const struct mrpc_param_vtable *mrpc_string_param_get_vtable()
{
	return &string_param_vtable;
}
