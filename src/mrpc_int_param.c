#include "private/mrpc_common.h"

#include "private/mrpc_int_param.h"
#include "private/mrpc_param.h"
#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

struct uint64_param
{
	uint64_t value;
};

static void delete_uint64_param(struct mrpc_param *param)
{
	struct uint64_param *uint64_param;

	uint64_param = (struct uint64_param *) mrpc_param_get_ctx(param);
	ff_free(uint64_param);
}

static enum ff_result read_uint64_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct uint64_param *uint64_param;
	enum ff_result result;

	uint64_param = (struct uint64_param *) mrpc_param_get_ctx(param);
	ff_assert(uint64_param->value == 0);
	result = mrpc_uint64_unserialize(&uint64_param->value, stream);
	return result;
}

static enum ff_result write_uint64_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct uint64_param *uint64_param;
	enum ff_result result;

	uint64_param = (struct uint64_param *) mrpc_param_get_ctx(param);
	result = mrpc_uint64_serialize(uint64_param->value, stream);
	return result;
}

static void get_uint64_param_value(struct mrpc_param *param, void **value)
{
	struct uint64_param *uint64_param;

	uint64_param = (struct uint64_param *) mrpc_param_get_ctx(param);
	*(uint64_t **) value = &uint64_param->value;
}

static void set_uint64_param_value(struct mrpc_param *param, const void *value)
{
	struct uint64_param *uint64_param;

	uint64_param = (struct uint64_param *) mrpc_param_get_ctx(param);
	ff_assert(uint64_param->value == 0);
	uint64_param->value = *(uint64_t *) value;
}

static uint32_t get_uint64_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct uint64_param *uint64_param;
	uint32_t hash_value;
	uint32_t tmp[2];

	uint64_param = (struct uint64_param *) mrpc_param_get_ctx(param);
	tmp[0] = (uint32_t) uint64_param->value;
	tmp[1] = (uint32_t) (uint64_param->value >> 32);
	hash_value = ff_hash_uint32(start_value, tmp, 2);
	return hash_value;
}

static const struct mrpc_param_vtable uint64_param_vtable =
{
	delete_uint64_param,
	read_uint64_param_from_stream,
	write_uint64_param_to_stream,
	get_uint64_param_value,
	set_uint64_param_value,
	get_uin64_param_hash
};

struct mrpc_param *mrpc_uint64_param_create()
{
	struct mrpc_param *param;
	struct uint64_param *uint64_param;

	uint64_param = (struct uint64_param *) ff_malloc(sizeof(*param));
	uint64_param->value = 0;

	param = mrpc_param_create(&uint64_param_vtable, uint64_param);
	return param;
}


struct int64_param
{
	int64_t value;
};

static void delete_int64_param(struct mrpc_param *param)
{
	struct int64_param *int64_param;

	int64_param = (struct int64_param *) mrpc_param_get_ctx(param);
	ff_free(int64_param);
}

static enum ff_result read_int64_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct int64_param *int64_param;
	enum ff_result result;

	int64_param = (struct int64_param *) mrpc_param_get_ctx(param);
	ff_assert(int64_param->value == 0);
	result = mrpc_int64_unserialize(&int64_param->value, stream);
	return result;
}

static enum ff_result write_int64_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct int64_param *int64_param;
	enum ff_result result;

	int64_param = (struct int64_param *) mrpc_param_get_ctx(param);
	result = mrpc_int64_serialize(int64_param->value, stream);
	return result;
}

static void get_int64_param_value(struct mrpc_param *param, void **value)
{
	struct int64_param *int64_param;

	int64_param = (struct int64_param *) mrpc_param_get_ctx(param);
	*(int64_t **) value = &int64_param->value;
}

static void set_int64_param_value(struct mrpc_param *param, const void *value)
{
	struct int64_param *int64_param;

	int64_param = (struct int64_param *) mrpc_param_get_ctx(param);
	ff_assert(int64_param->value == 0);
	int64_param->value = *(int64_t *) value;
}

static uint32_t get_int64_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct int64_param *int64_param;
	uint32_t hash_value;
	uint32_t tmp[2];

	int64_param = (struct int64_param *) mrpc_param_get_ctx(param);
	tmp[0] = (uint32_t) int64_param->value;
	tmp[1] = (uint32_t) (int64_param->value >> 32);
	hash_value = ff_hash_uint32(start_value, tmp, 2);
	return hash_value;
}

static const struct mrpc_param_vtable int64_param_vtable =
{
	delete_int64_param,
	read_int64_param_from_stream,
	write_int64_param_to_stream,
	get_int64_param_value,
	set_int64_param_value,
	get_uin64_param_hash
};

struct mrpc_param *mrpc_int64_param_create()
{
	struct mrpc_param *param;
	struct int64_param *int64_param;

	int64_param = (struct int64_param *) ff_malloc(sizeof(*param));
	int64_param->value = 0;

	param = mrpc_param_create(&int64_param_vtable, int64_param);
	return param;
}


struct uint32_param
{
	uint32_t value;
};

static void delete_uint32_param(struct mrpc_param *param)
{
	struct uint32_param *uint32_param;

	uint32_param = (struct uint32_param *) mrpc_param_get_ctx(param);
	ff_free(uint32_param);
}

static enum ff_result read_uint32_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct uint32_param *uint32_param;
	enum ff_result result;

	uint32_param = (struct uint32_param *) mrpc_param_get_ctx(param);
	ff_assert(uint32_param->value == 0);
	result = mrpc_uint32_unserialize(&uint32_param->value, stream);
	return result;
}

static enum ff_result write_uint32_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct uint32_param *uint32_param;
	enum ff_result result;

	uint32_param = (struct uint32_param *) mrpc_param_get_ctx(param);
	result = mrpc_uint32_serialize(uint32_param->value, stream);
	return result;
}

static void get_uint32_param_value(struct mrpc_param *param, void **value)
{
	struct uint32_param *uint32_param;

	uint32_param = (struct uint32_param *) mrpc_param_get_ctx(param);
	*(uint32_t **) value = &uint32_param->value;
}

static void set_uint32_param_value(struct mrpc_param *param, const void *value)
{
	struct uint32_param *uint32_param;

	uint32_param = (struct uint32_param *) mrpc_param_get_ctx(param);
	ff_assert(uint32_param->value == 0);
	uint32_param->value = *(uint32_t *) value;
}

static uint32_t get_uint32_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct uint32_param *uint32_param;
	uint32_t hash_value;

	uint32_param = (struct uint32_param *) mrpc_param_get_ctx(param);
	hash_value = ff_hash_uint32(start_value, &uint32_param->value, 1);
	return hash_value;
}

static const struct mrpc_param_vtable uint32_param_vtable =
{
	delete_uint32_param,
	read_uint32_param_from_stream,
	write_uint32_param_to_stream,
	get_uint32_param_value,
	set_uint32_param_value,
	get_uin32_param_hash
};

struct mrpc_param *mrpc_uint32_param_create()
{
	struct mrpc_param *param;
	struct uint32_param *uint32_param;

	uint32_param = (struct uint32_param *) ff_malloc(sizeof(*param));
	uint32_param->value = 0;

	param = mrpc_param_create(&uint32_param_vtable, uint32_param);
	return param;
}


struct int32_param
{
	int32_t value;
};

static void delete_int32_param(struct mrpc_param *param)
{
	struct int32_param *int32_param;

	int32_param = (struct int32_param *) mrpc_param_get_ctx(param);
	ff_free(int32_param);
}

static enum ff_result read_int32_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct int32_param *int32_param;
	enum ff_result result;

	int32_param = (struct int32_param *) mrpc_param_get_ctx(param);
	ff_assert(int32_param->value == 0);
	result = mrpc_int32_unserialize(&int32_param->value, stream);
	return result;
}

static enum ff_result write_int32_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct int32_param *int32_param;
	enum ff_result result;

	int32_param = (struct int32_param *) mrpc_param_get_ctx(param);
	result = mrpc_int32_serialize(int32_param->value, stream);
	return result;
}

static void get_int32_param_value(struct mrpc_param *param, void **value)
{
	struct int32_param *int32_param;

	int32_param = (struct int32_param *) mrpc_param_get_ctx(param);
	*(int32_t **) value = &int32_param->value;
}

static void set_int32_param_value(struct mrpc_param *param, const void *value)
{
	struct int32_param *int32_param;

	int32_param = (struct int32_param *) mrpc_param_get_ctx(param);
	ff_assert(int32_param->value == 0);
	int32_param->value = *(int32_t *) value;
}

static uint32_t get_int32_param_hash(const struct mrpc_param *param, uint32_t start_value)
{
	struct int32_param *int32_param;
	uint32_t hash_value;

	int32_param = (struct int32_param *) mrpc_param_get_ctx(param);
	hash_value = ff_hash_uint32(start_value, &int32_param->value, 1);
	return hash_value;
}

static const struct mrpc_param_vtable int32_param_vtable =
{
	delete_int32_param,
	read_int32_param_from_stream,
	write_int32_param_to_stream,
	get_int32_param_value,
	set_int32_param_value,
	get_uin32_param_hash
};

struct mrpc_param *mrpc_int32_param_create()
{
	struct mrpc_param *param;
	struct int32_param *int32_param;

	int32_param = (struct int32_param *) ff_malloc(sizeof(*param));
	int32_param->value = 0;

	param = mrpc_param_create(&int32_param_vtable, int32_param);
	return param;
}
