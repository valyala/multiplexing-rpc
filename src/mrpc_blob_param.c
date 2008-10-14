#include "private/mrpc_common.h"

#include "private/mrpc_blob_param.h"
#include "private/mrpc_param.h"
#include "private/mrpc_blob.h"
#include "private/mrpc_blob_serialization.h"
#include "ff/ff_stream.h"
#include "ff/ff_log.h"

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

static int read_blob_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct blob_param *blob_param;
	int is_success;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value == NULL);
	is_success = mrpc_blob_unserialize(&blob_param->value, stream);
	if (is_success)
	{
		ff_assert(blob_param->value != NULL);
	}
	return is_success;
}

static int write_blob_param_to_stream(const struct mrpc_param *param, struct ff_stream *stream)
{
	struct blob_param *blob_param;
	int is_success;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value != NULL);
	is_success = mrpc_blob_serialize(blob_param->value, stream);
	return is_success;
}

static void get_blob_param_value(const struct mrpc_param *param, void **value)
{
	struct blob_param *blob_param;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value != NULL);
	*(struct ff_blob **) value = blob_param->value;
}

static void set_blob_param_value(struct mrpc_param *param, const void *value)
{
	struct blob_param *blob_param;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value == NULL);
	blob_param->value = (struct ff_blob *) value;
}

static uint32_t get_blob_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct blob_param *blob_param;
	struct ff_stream *stream;
	uint32_t hash_value;
	int is_success = 0;

	blob_param = (struct blob_param *) mrpc_param_get_ctx(param);
	ff_assert(blob_param->value != NULL);
	hash_value = start_value;
	stream = mrpc_blob_open_stream(blob_param->value, MRPC_BLOB_READ);
	if (stream != NULL)
	{
		int blob_size;

		blob_size = mrpc_blob_get_size(blob_param->value);
		is_success = ff_stream_get_hash(stream, blob_size, hash_value, &hash_value);
		ff_stream_delete(stream);
	}

	if (!is_success)
	{
		ff_log_warning(L"cannot calculate hash value for the blob. See previous message for details");
	}
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
