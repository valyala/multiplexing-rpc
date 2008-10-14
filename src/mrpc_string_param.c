#include "private/mrpc_common.h"

#include "private/mrpc_string_param.h"
#include "private/mrpc_param.h"
#include "private/mrpc_string_serialization.h"
#include "ff/ff_string.h"
#include "ff/ff_stream.h"

struct string_param
{
	struct ff_string *value;
};

static void delete_string_param(struct mrpc_param *param)
{
	struct string_param *string_param;

	string_param = (struct string_param *) mrpc_param_get_ctx(param);
	if (string_param->value != NULL)
	{
		ff_string_dec_ref(string_param->value);
	}
	ff_free(string_param);
}

static int read_string_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct string_param *string_param;
	int is_success;

	string_param = (struct string_param *) mrpc_param_get_ctx(param);
	ff_assert(string_param->value == NULL);
	is_success = mrpc_string_unserialize(&string_param->value, stream);
	if (is_success)
	{
		ff_assert(string_param->value != NULL);
	}
	return is_success;
}

static int write_string_param_to_stream(const struct mrpc_param *param, struct ff_stream *stream)
{
	struct string_param *string_param;
	int is_success;

	string_param = (struct string_param *) mrpc_param_get_ctx(param);
	ff_assert(string_param->value != NULL);
	is_success = mrpc_string_serialize(string_param->value, stream);
	return is_success;
}

static void get_string_param_value(const struct mrpc_param *param, void **value)
{
	struct string_param *string_param;

	string_param = (struct string_param *) mrpc_param_get_ctx(param);
	ff_assert(string_param->value != NULL);
	*(struct ff_string **) value = string_param->value;
}

static void set_string_param_value(struct mrpc_param *param, const void *value)
{
	struct string_param *string_param;

	string_param = (struct string_param *) mrpc_param_get_ctx(param);
	ff_assert(string_param->value == NULL);
	string_param->value = (struct ff_string *) value;
}

static uint32_t get_string_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct string_param *string_param;
	uint32_t hash_value;

	string_param = (struct string_param *) mrpc_param_get_ctx(param);
	ff_assert(string_param->value != NULL);
	hash_value = ff_string_get_hash(string_param->value, start_value);
	return hash_value;
}

static const struct mrpc_param_vtable string_param_vtable =
{
	delete_string_param,
	read_string_param_from_stream,
	write_string_param_to_stream,
	get_string_param_value,
	set_string_param_value,
	get_string_param_hash
};

struct mrpc_param *mrpc_string_param_create()
{
	struct mrpc_param *param;
	struct string_param *string_param;

	string_param = (struct string_param *) ff_malloc(sizeof(*string_param));
	string_param->value = NULL;

	param = mrpc_param_create(&string_param_vtable, string_param);
	return param;
}
