#include "private/mrpc_common.h"

#include "private/mrpc_blob_param.h"
#include "private/mrpc_param.h"
#include "private/mrpc_blob.h"
#include "private/mrpc_blob_serialization.h"
#include "ff/ff_stream.h"

struct blob_param
{
	struct mrpc_blob *value;
};

static void delete_blob_param(struct mrpc_param *param)
{
	struct blob_param *blob_param;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	if (blob_param->value != NULL)
	{
		mrpc_blob_dec_ref(blob_param->value);
	}
	ff_free(blob_param);
}

static enum ff_result read_blob_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct blob_param *blob_param;
	enum ff_result result;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value == NULL);
	result = mrpc_blob_unserialize(&blob_param->value, stream);
	if (result == FF_SUCCESS)
	{
		ff_assert(blob_param->value != NULL);
	}
	else
	{
		ff_log_warning(L"cannot read blob param=%p from the stream=%p. See previous messages for more info", param, stream);
	}
	return result;
}

static enum ff_result write_blob_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct blob_param *blob_param;
	enum ff_result result;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value != NULL);
	result = mrpc_blob_serialize(blob_param->value, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_warning(L"cannot write blob param=%p to the stream=%p. See previous messages for more info", param, stream);
	}
	return result;
}

static void get_blob_param_value(struct mrpc_param *param, void **value)
{
	struct blob_param *blob_param;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value != NULL);
	*(struct mrpc_blob **) value = blob_param->value;
	/* there is no need to call the mrpc_blob_inc_ref() here,
	 * because the caller should be responsible for doing so
	 * only is such cases as passing the blob out of the caller's scope.
	 */
}

static void set_blob_param_value(struct mrpc_param *param, const void *value)
{
	struct blob_param *blob_param;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value == NULL);
	/* there is no need to call the mrpc_blob_inc_ref() here,
	 * because the caller should be responsible for doing so
	 * only in such cases as passing the blob out of the caller's scope.
	 */
	blob_param->value = (struct mrpc_blob *) value;
}

static uint32_t get_blob_param_hash(struct mrpc_param *param, uint32_t start_value)
{
	struct blob_param *blob_param;
	uint32_t hash_value;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value != NULL);
	hash_value = mrpc_blob_get_hash(blob_param->value, start_value);
	return hash_value;
}

static const struct mrpc_param_vtable blob_param_vtable =
{
	delete_blob_param,
	read_blob_param_from_stream,
	write_blob_param_to_stream,
	get_blob_param_value,
	set_blob_param_value,
	get_blob_param_hash
};

struct mrpc_param *mrpc_blob_param_create()
{
	struct mrpc_param *param;
	struct blob_param *blob_param;

	blob_param = (struct blob_param *) ff_malloc(sizeof(*blob_param));
	blob_param->value = NULL;

	param = mrpc_param_create(&blob_param_vtable, blob_param);
	return param;
}
