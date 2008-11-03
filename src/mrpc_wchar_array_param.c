#include "private/mrpc_common.h"

#include "private/mrpc_wchar_array_param.h"
#include "private/mrpc_wchar_array_serialization.h"
#include "private/mrpc_wchar_array.h"
#include "private/mrpc_param.h"
#include "ff/ff_stream.h"

struct wchar_array_param
{
	struct mrpc_wchar_array *value;
};

static void delete_wchar_array_param(struct mrpc_param *param)
{
	struct wchar_array_param *wchar_array_param;

	wchar_array_param = (struct wchar_array_param *) mrpc_param_get_ctx(param);
	if (wchar_array_param->value != NULL)
	{
		mrpc_wchar_array_dec_ref(wchar_array_param->value);
	}
	ff_free(wchar_array_param);
}

static enum ff_result read_wchar_array_param_from_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct wchar_array_param *wchar_array_param;
	enum ff_result result;

	wchar_array_param = (struct wchar_array_param *) mrpc_param_get_ctx(param);
	ff_assert(wchar_array_param->value == NULL);
	result = mrpc_wchar_array_unserialize(&wchar_array_param->value, stream);
	if (result == FF_SUCCESS)
	{
		ff_assert(wchar_array_param->value != NULL);
	}
	return result;
}

static enum ff_result write_wchar_array_param_to_stream(struct mrpc_param *param, struct ff_stream *stream)
{
	struct wchar_array_param *wchar_array_param;
	enum ff_result result;

	wchar_array_param = (struct wchar_array_param *) mrpc_param_get_ctx(param);
	ff_assert(wchar_array_param->value != NULL);
	result = mrpc_wchar_array_serialize(wchar_array_param->value, stream);
	return result;
}

static void get_wchar_array_param_value(struct mrpc_param *param, void **value)
{
	struct wchar_array_param *wchar_array_param;

	wchar_array_param = (struct wchar_array_param *) mrpc_param_get_ctx(param);
	ff_assert(wchar_array_param->value != NULL);
	*(struct mrpc_wchar_array **) value = wchar_array_param->value;
	/* there is no need to call the mrpc_wchar_array_inc_ref() here,
	 * because the caller should be responsible for doing so
	 * only is such cases as passing the blob out of the caller's scope.
	 */
}

static void set_wchar_array_param_value(struct mrpc_param *param, const void *value)
{
	struct wchar_array_param *wchar_array_param;

	wchar_array_param = (struct wchar_array_param *) mrpc_param_get_ctx(param);
	ff_assert(wchar_array_param->value == NULL);
	/* there is no need to call the mrpc_wchar_array_inc_ref() here,
	 * because the caller should be responsible for doing so
	 * only is such cases as passing the blob out of the caller's scope.
	 */
	wchar_array_param->value = (struct mrpc_wchar_array *) value;
}

static uint32_t get_wchar_array_param_hash(struct mrpc_param *param, uint32_t start_value)
{
	struct wchar_array_param *wchar_array_param;
	uint32_t hash_value;

	wchar_array_param = (struct wchar_array_param *) mrpc_param_get_ctx(param);
	ff_assert(wchar_array_param->value != NULL);
	hash_value = mrpc_wchar_array_get_hash(wchar_array_param->value, start_value);
	return hash_value;
}

static const struct mrpc_param_vtable wchar_array_param_vtable =
{
	delete_wchar_array_param,
	read_wchar_array_param_from_stream,
	write_wchar_array_param_to_stream,
	get_wchar_array_param_value,
	set_wchar_array_param_value,
	get_wchar_array_param_hash
};

struct mrpc_param *mrpc_wchar_array_param_create()
{
	struct mrpc_param *param;
	struct wchar_array_param *wchar_array_param;

	wchar_array_param = (struct wchar_array_param *) ff_malloc(sizeof(*wchar_array_param));
	wchar_array_param->value = NULL;

	param = mrpc_param_create(&wchar_array_param_vtable, wchar_array_param);
	return param;
}
