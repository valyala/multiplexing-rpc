#include "private/mrpc_common.h"

#include "private/mrpc_wchar_array.h"

struct mrpc_wchar_array
{
	const wchar_t *value;
	int len;
	int ref_cnt;
};

static void delete_wchar_array(struct mrpc_wchar_array *wchar_array)
{
	ff_free(wchar_array->value);
	ff_free(wchar_array);
}

struct mrpc_wchar_array *mrpc_wchar_array_create(const wchar_t *value, int len)
{
	struct mrpc_wchar_array *wchar_array;

	ff_assert(len >= 0);
	ff_assert(value != NULL);

	wchar_array = (struct mrpc_wchar_array *) ff_malloc(sizeof(*wchar_array));
	wchar_array->value = value;
	wchar_array->len = len;
	wchar_array->ref_cnt = 1;

	return wchar_array;
}

void mrpc_wchar_array_inc_ref(struct mrpc_wchar_array *wchar_array)
{
	wchar_array->ref_cnt++;
	ff_assert(wchar_array->ref_cnt > 0);
}

void mrpc_wchar_array_dec_ref(struct mrpc_wchar_array *wchar_array)
{
	ff_assert(wchar_array->ref_cnt > 0);
	wchar_array->ref_cnt--;
	if (wchar_array->ref_cnt == 0)
	{
		delete_wchar_array(wchar_array);
	}
}

const wchar_t *mrpc_wchar_array_get_value(struct mrpc_wchar_array *wchar_array)
{
	return wchar_array->value;
}

int mrpc_wchar_array_get_len(struct mrpc_wchar_array *wchar_array)
{
	return wchar_array->len;
}
