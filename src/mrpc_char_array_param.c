#include "private/mrpc_common.h"

#include "private/mrpc_char_array_param.h"
#include "private/mrpc_char_array_serialization.h"
#include "private/mrpc_char_array.h"
#include "private/mrpc_param.h"
#include "ff/ff_stream.h"

struct char_array_param
{
	struct mrpc_char_array *value;
};

static void delete_char_array_param(struct mrpc_param *param)
{
	struct char_array_param *char_array_param;

	char_array_param = (struct char_array_param *) mrpc_param_get_ctx(param);
	if (char_array_param->value != NULL)
	{
		mrpc_char_array_dec_ref(char_array_param->value);
	}
	ff_free(char_array_param);
}

static enum ff_result read_char_array_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct char_array_param *char_array_param;
	enum ff_result result;

	char_array_param = (struct char_array_param *) mrpc_param_get_ctx(param);
	ff_assert(char_array_param->value == NULL);
	result = mrpc_char_array_unserialize(&char_array_param->value, stream);
	if (result == FF_SUCCESS)
	{
		ff_assert(char_array_param->value != NULL);
	}
	return result;
}

static enum ff_result write_char_array_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct char_array_param *char_array_param;
	enum ff_result result;

	char_array_param = (struct char_array_param *) mrpc_param_get_ctx(param);
	ff_assert(char_array_param->value != NULL);
	result = mrpc_char_array_serialize(char_array_param->value, stream);
	return result;
}

static void get_char_array_param_value(struct mrpc_param *param, void **value)
{
	struct char_array_param *char_array_param;

	char_array_param = (struct char_array_param *) mrpc_param_get_ctx(param);
	ff_assert(char_array_param->value != NULL);
	*(struct mrpc_char_array **) value = char_array_param->value;
}

static void set_char_array_param_value(struct mrpc_param *param, const void *value)
{
	struct char_array_param *char_array_param;

	char_array_param = (struct char_array_param *) mrpc_param_get_ctx(param);
	ff_assert(char_array_param->value == NULL);
	char_array_param->value = (struct mrpc_char_array *) value;
}

static uint32_t get_char_array_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct char_array_param *char_array_param;
	uint32_t hash_value;

	char_array_param = (struct char_array_param *) mrpc_param_get_ctx(param);
	ff_assert(char_array_param->value != NULL);
	hash_value = mrpc_char_array_get_hash(char_array_param->value, start_value);
	return hash_value;
}

static const struct mrpc_param_vtable char_array_param_vtable =
{
	delete_char_array_param,
	read_char_array_param_from_stream,
	write_char_array_param_to_stream,
	get_char_array_param_value,
	set_char_array_param_value,
	get_char_array_param_hash
};

struct mrpc_param *mrpc_char_array_param_create()
{
	struct mrpc_param *param;
	struct char_array_param *char_array_param;

	char_array_param = (struct char_array_param *) ff_malloc(sizeof(*char_array_param));
	char_array_param->value = NULL;

	param = mrpc_param_create(&char_array_param_vtable, char_array_param);
	return param;
}
