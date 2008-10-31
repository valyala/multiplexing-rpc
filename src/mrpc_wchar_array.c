#include "private/mrpc_common.h"

#include "private/mrpc_wchar_array.h"
#include "ff/ff_hash.h"

struct mrpc_wchar_array
{
	const wchar_t *value;
	int len;
	int ref_cnt;
};

static struct mrpc_wchar_array *create_wchar_array(const wchar_t *value, int len)
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

	wchar_array = create_wchar_array(value, len);

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

uint32_t mrpc_wchar_array_get_hash(struct mrpc_wchar_array *wchar_array, uint32_t start_value)
{
	uint32_t hash_value;

	if (sizeof(wchar_t) == 4)
	{
		uint16_t *buf;

		buf = (uint16_t *) ff_calloc(wchar_array->len, sizeof(buf[0]));
		for (i = 0; i < wchar_array->len; i++)
		{
			wchar_t ch;

			ch = wchar_array->value[i];
			ff_assert(ch < 0x10000);
			buf[i] = (uint16_t) ch;
		}
		hash_value = ff_hash_uint16(start_value, buf, wchar_array->len);
		ff_free(buf);
	}
	else
	{
		ff_assert(sizeof(wchar_t) == 2);
		hash_value = ff_hash_uint16(start_value, wchar_array->value, wchar_array->len);
	}
	return hash_value;
}
