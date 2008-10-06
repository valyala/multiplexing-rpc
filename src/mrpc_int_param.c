#include "private/mrpc_common.h"

#include "private/mrpc_int_param.h"
#include "private/mrpc_param_vtable.h"
#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

struct uint64_param
{
	uint64_t value;
};

static void *create_uint64_param()
{
	struct uint64_param *param;

	param = (struct uint64_param *) ff_malloc(sizeof(*param));
	param->value = 0;
	return param;
}

static void delete_uin64_param(void *ctx)
{
	struct uint64_param *param;

	param = (struct uint64_param *) ctx;
	ff_free(param);
}

static int read_uint64_param_from_stream(void *ctx, struct ff_stream *stream)
{
	struct uint64_param *param;
	int is_success;

	param = (struct uint64_param *) ctx;
	ff_assert(param->value == 0);
	is_success = mrpc_uint64_unserialize(&param->value, stream);
	return is_success;
}

static int write_uint64_param_to_stream(const void *ctx, struct ff_stream *stream)
{
	struct uint64_param *param;
	int is_success;

	param = (struct uint64_param *) ctx;
	is_success = mrpc_uint64_serialize(param->value, stream);
	return is_success;
}

static void get_uint64_param_value(const void *ctx, void **value)
{
	struct uint64_param *param;

	param = (struct uint64_param *) ctx;
	*(uint64_t **) value = &param->value;
}

static void set_uint64_param_value(void *ctx, const void *value)
{
	struct uint64_param *param;

	param = (struct uint64_param *) ctx;
	ff_assert(param->value == 0);
	param->value = *(uint64_t *) value;
}

static uint32_t get_uint64_param_hash(const void *ctx, uint32_t start_value)
{
	struct uint64_param *param;
	uint32_t hash;

	param = (struct uint64_param *) ctx;
	hash = ff_hash_uint32(start_value, &param->value, 2);
	return hash;
}

static const struct mrpc_param_vtable uint64_param_vtable =
{
	create_uint64_param,
	delete_uint64_param,
	read_uint64_param_from_stream,
	write_uint64_param_to_stream,
	get_uin64_param_value,
	set_uint64_param_value,
	get_uin64_param_hash
};

const struct mrpc_param_vtable *mrpc_uint64_param_get_vtable()
{
	return &uint64_param_vtable;
}


struct int64_param
{
	int64_t value;
};

static void *create_int64_param()
{
	struct int64_param *param;

	param = (struct int64_param *) ff_malloc(sizeof(*param));
	param->value = 0;
	return param;
}

static void delete_uin64_param(void *ctx)
{
	struct int64_param *param;

	param = (struct int64_param *) ctx;
	ff_free(param);
}

static int read_int64_param_from_stream(void *ctx, struct ff_stream *stream)
{
	struct int64_param *param;
	int is_success;

	param = (struct int64_param *) ctx;
	ff_assert(param->value == 0);
	is_success = mrpc_int64_unserialize(&param->value, stream);
	return is_success;
}

static int write_int64_param_to_stream(const void *ctx, struct ff_stream *stream)
{
	struct int64_param *param;
	int is_success;

	param = (struct int64_param *) ctx;
	is_success = mrpc_int64_serialize(param->value, stream);
	return is_success;
}

static void get_int64_param_value(const void *ctx, void **value)
{
	struct int64_param *param;

	param = (struct int64_param *) ctx;
	*(int64_t **) value = &param->value;
}

static void set_int64_param_value(void *ctx, const void *value)
{
	struct int64_param *param;

	param = (struct int64_param *) ctx;
	ff_assert(param->value == 0);
	param->value = *(int64_t *) value;
}

static uint32_t get_int64_param_hash(const void *ctx, uint32_t start_value)
{
	struct int64_param *param;
	uint32_t hash;

	param = (struct int64_param *) ctx;
	hash = ff_hash_uint32(start_value, &param->value, 2);
	return hash;
}

static const struct mrpc_param_vtable int64_param_vtable =
{
	create_int64_param,
	delete_int64_param,
	read_int64_param_from_stream,
	write_int64_param_to_stream,
	get_uin64_param_value,
	set_int64_param_value,
	get_uin64_param_hash
};

const struct mrpc_param_vtable *mrpc_int64_param_get_vtable()
{
	return &int64_param_vtable;
}


struct uint32_param
{
	uint32_t value;
};

static void *create_uint32_param()
{
	struct uint32_param *param;

	param = (struct uint32_param *) ff_malloc(sizeof(*param));
	param->value = 0;
	return param;
}

static void delete_uin32_param(void *ctx)
{
	struct uint32_param *param;

	param = (struct uint32_param *) ctx;
	ff_free(param);
}

static int read_uint32_param_from_stream(void *ctx, struct ff_stream *stream)
{
	struct uint32_param *param;
	int is_success;

	param = (struct uint32_param *) ctx;
	ff_assert(param->value == 0);
	is_success = mrpc_uint32_unserialize(&param->value, stream);
	return is_success;
}

static int write_uint32_param_to_stream(const void *ctx, struct ff_stream *stream)
{
	struct uint32_param *param;
	int is_success;

	param = (struct uint32_param *) ctx;
	is_success = mrpc_uint32_serialize(param->value, stream);
	return is_success;
}

static void get_uint32_param_value(const void *ctx, void **value)
{
	struct uint32_param *param;

	param = (struct uint32_param *) ctx;
	*(uint32_t **) value = &param->value;
}

static void set_uint32_param_value(void *ctx, const void *value)
{
	struct uint32_param *param;

	param = (struct uint32_param *) ctx;
	ff_assert(param->value == 0);
	param->value = *(uint32_t *) value;
}

static uint32_t get_uint32_param_hash(const void *ctx, uint32_t start_value)
{
	struct uint32_param *param;
	uint32_t hash;

	param = (struct uint32_param *) ctx;
	hash = ff_hash_uint32(start_value, &param->value, 2);
	return hash;
}

static const struct mrpc_param_vtable uint32_param_vtable =
{
	create_uint32_param,
	delete_uint32_param,
	read_uint32_param_from_stream,
	write_uint32_param_to_stream,
	get_uin32_param_value,
	set_uint32_param_value,
	get_uin32_param_hash
};

const struct mrpc_param_vtable *mrpc_uint32_param_get_vtable()
{
	return &uint32_param_vtable;
}


struct int32_param
{
	int32_t value;
};

static void *create_int32_param()
{
	struct int32_param *param;

	param = (struct int32_param *) ff_malloc(sizeof(*param));
	param->value = 0;
	return param;
}

static void delete_uin32_param(void *ctx)
{
	struct int32_param *param;

	param = (struct int32_param *) ctx;
	ff_free(param);
}

static int read_int32_param_from_stream(void *ctx, struct ff_stream *stream)
{
	struct int32_param *param;
	int is_success;

	param = (struct int32_param *) ctx;
	ff_assert(param->value == 0);
	is_success = mrpc_int32_unserialize(&param->value, stream);
	return is_success;
}

static int write_int32_param_to_stream(const void *ctx, struct ff_stream *stream)
{
	struct int32_param *param;
	int is_success;

	param = (struct int32_param *) ctx;
	is_success = mrpc_int32_serialize(param->value, stream);
	return is_success;
}

static void get_int32_param_value(const void *ctx, void **value)
{
	struct int32_param *param;

	param = (struct int32_param *) ctx;
	*(int32_t **) value = &param->value;
}

static void set_int32_param_value(void *ctx, const void *value)
{
	struct int32_param *param;

	param = (struct int32_param *) ctx;
	ff_assert(param->value == 0);
	param->value = *(int32_t *) value;
}

static uint32_t get_int32_param_hash(const void *ctx, uint32_t start_value)
{
	struct int32_param *param;
	uint32_t hash;

	param = (struct int32_param *) ctx;
	hash = ff_hash_uint32(start_value, &param->value, 2);
	return hash;
}

static const struct mrpc_param_vtable int32_param_vtable =
{
	create_int32_param,
	delete_int32_param,
	read_int32_param_from_stream,
	write_int32_param_to_stream,
	get_uin32_param_value,
	set_int32_param_value,
	get_uin32_param_hash
};

const struct mrpc_param_vtable *mrpc_int32_param_get_vtable()
{
	return &int32_param_vtable;
}
