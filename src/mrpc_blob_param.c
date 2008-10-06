#include "private/mrpc_common.h"

#include "private/mrpc_blob_param.h"
#include "private/mrpc_param_vtable.h"
#include "private/mrpc_blob.h"
#include "private/mrpc_blob_serialization.h"
#include "ff/ff_stream.h"
#include "ff/ff_log.h"

struct blob_param
{
	struct mrpc_blob *blob;
};

static void *create_blob_param()
{
	struct blob_param *param;

	param = (struct blob_param *) ff_malloc(sizeof(*param));
	param->blob = NULL;
	return param;
}

static void delete_blob_param(void *ctx)
{
	struct blob_param *param;

	param = (struct blob_param *) ctx;
	if (param->blob != NULL)
	{
		mrpc_blob_dec_ref(param->blob);
	}
	ff_free(param);
}

static int read_blob_param_from_stream(void *ctx, struct ff_stream *stream)
{
	struct blob_param *param;
	int is_success;

	param = (struct blob_param *) ctx;
	ff_assert(param->blob == NULL);
	is_success = mrpc_blob_unserialize(&param->blob, stream);
	if (is_success)
	{
		ff_assert(param->blob != NULL);
	}
	return is_success;
}

static int write_blob_param_to_stream(const void *ctx, struct ff_stream *stream)
{
	struct blob_param *param;
	int is_success;

	param = (struct blob_param *) ctx;
	ff_assert(param->blob != NULL);
	is_success = mrpc_blob_serialize(param->blob, stream);
	return is_success;
}

static void get_blob_param_value(const void *ctx, void **value)
{
	struct blob_param *param;

	param = (struct blob_param *) ctx;
	ff_assert(param->blob != NULL);
	*(struct ff_blob **) value = param->blob;
}

static void set_blob_param_value(void *ctx, const void *value)
{
	struct blob_param *param;

	param = (struct blob_param *) ctx;
	ff_assert(param->blob == NULL);
	param->blob = (struct ff_blob *) value;
}

static uint32_t get_blob_param_hash(const void *ctx, uint32_t start_value)
{
	struct blob_param *param;
	struct ff_stream *stream;
	uint32_t hash;
	int is_success = 0;

	param = (struct blob_param *) ctx;
	ff_assert(param->blob != NULL);
	hash = start_value;
	stream = mrpc_blob_open_stream(param->blob, MRPC_BLOB_READ);
	if (stream != NULL)
	{
		int blob_size;

		blob_size = mrpc_blob_get_size(param->blob);
		is_success = ff_stream_get_hash(stream, blob_size, hash, &hash);
		ff_stream_delete(stream);
	}

	if (!is_success)
	{
		ff_log_warning(L"cannot calculate hash value for the blob. See previous message for details");
	}
	return hash;
}

static const struct mrpc_param_vtable blob_param_vtable =
{
	create_blob_param,
	delete_blob_param,
	read_blob_param_from_stream,
	write_blob_param_to_stream,
	get_blob_param_value,
	set_blob_param_value,
	get_blob_param_hash
};

const struct mrpc_param_vtable *mrpc_blob_param_get_vtable()
{
	return &blob_param_vtable;
}
