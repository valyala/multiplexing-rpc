#include "private/mrpc_common.h"

#include "private/mrpc_wchar_array.h"
#include "private/mrpc_int.h"
#include "ff/ff_hash.h"

/* the maximum wchar_array length.
 * This limit is security-related - it doesn't allow to send extremely long wchar arrays
 * in order to trigger out of memory errors.
 * The ((1 << 14) - 1) length was chosen, because it can be encoded into maximum two bytes
 * by the mrpc_uint32_serialize() function
 */
#define MAX_WCHAR_ARRAY_LENGTH ((1 << 14) - 1)

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
	ff_assert(len <= MAX_WCHAR_ARRAY_LENGTH);
	ff_assert(value != NULL);

	wchar_array = (struct mrpc_wchar_array *) ff_malloc(sizeof(*wchar_array));
	wchar_array->value = value;
	wchar_array->len = len;
	wchar_array->ref_cnt = 1;

	return wchar_array;
}

static void delete_wchar_array(struct mrpc_wchar_array *wchar_array)
{
	ff_assert(wchar_array->ref_cnt == 0);
	ff_assert(wchar_array->value != NULL);

	ff_free((void *) wchar_array->value);
	ff_free(wchar_array);
}

struct mrpc_wchar_array *mrpc_wchar_array_create(const wchar_t *value, int len)
{
	struct mrpc_wchar_array *wchar_array;

	ff_assert(len >= 0);
	ff_assert(len <= MAX_WCHAR_ARRAY_LENGTH);
	ff_assert(value != NULL);

	wchar_array = create_wchar_array(value, len);
	return wchar_array;
}

void mrpc_wchar_array_inc_ref(struct mrpc_wchar_array *wchar_array)
{
	wchar_array->ref_cnt++;
	ff_assert(wchar_array->ref_cnt > 1);
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
	ff_assert(wchar_array->ref_cnt > 0);
	ff_assert(wchar_array->value != NULL);

	return wchar_array->value;
}

int mrpc_wchar_array_get_len(struct mrpc_wchar_array *wchar_array)
{
	ff_assert(wchar_array->ref_cnt > 0);
	ff_assert(wchar_array->len >= 0);
	ff_assert(wchar_array->len <= MAX_WCHAR_ARRAY_LENGTH);

	return wchar_array->len;
}

uint32_t mrpc_wchar_array_get_hash(struct mrpc_wchar_array *wchar_array, uint32_t start_value)
{
	const wchar_t *value;
	uint32_t hash_value;
	int len;

	ff_assert(wchar_array->ref_cnt > 0);

	value = mrpc_wchar_array_get_value(wchar_array);
	len = mrpc_wchar_array_get_len(wchar_array);

	/* the following test must be eliminated by optimizing compiler */
	if (sizeof(wchar_t) == 4)
	{
		uint16_t *buf;
		int i;

		/* this code is required for receiving the same hash value for the
		 * wchar_array as on the architectures where sizeof(wchar_t) == 2.
		 */
		buf = (uint16_t *) ff_calloc(len, sizeof(buf[0]));
		for (i = 0; i < len; i++)
		{
			wchar_t ch;

			ch = value[i];
			ff_assert(ch < 0x10000);
			buf[i] = (uint16_t) ch;
		}
		hash_value = ff_hash_uint16(start_value, buf, len);
		ff_free(buf);
	}
	else
	{
		ff_assert(sizeof(wchar_t) == 2);
		hash_value = ff_hash_uint16(start_value, (uint16_t *) value, len);
	}
	return hash_value;
}

enum ff_result mrpc_wchar_array_serialize(struct mrpc_wchar_array *wchar_array, struct ff_stream *stream)
{
	int i;
	int len;
	const wchar_t *value;
	enum ff_result result = FF_FAILURE;

	mrpc_wchar_array_inc_ref(wchar_array);
	len = mrpc_wchar_array_get_len(wchar_array);

	result = mrpc_uint32_serialize((uint32_t) len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot serialize wchar_array=%p length len=%d into the stream=%p. See previous messages for more info", wchar_array, len, stream);
		goto end;
	}

	value = mrpc_wchar_array_get_value(wchar_array);
	for (i = 0; i < len; i++)
	{
		uint32_t ch;

		ch = (uint32_t) value[i];
		ff_assert(ch < 0x10000);
		result = mrpc_uint32_serialize(ch, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot serialize character ch=%lu number %d from the wchar_array=%p into the stream=%p. See previous messages for more info", ch, i, wchar_array, stream);
			goto end;
		}
	}

end:
	mrpc_wchar_array_dec_ref(wchar_array);
	return result;
}

enum ff_result mrpc_wchar_array_unserialize(struct mrpc_wchar_array **wchar_array, struct ff_stream *stream)
{
	uint32_t i;
	uint32_t u_len;
	wchar_t *value;
	enum ff_result result;

	result = mrpc_uint32_unserialize(&u_len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot unserialize wchar_array length from the stream=%p. See previous messages for more info", stream);
		goto end;
	}
	if (u_len > MAX_WCHAR_ARRAY_LENGTH)
	{
		ff_log_debug(L"unserialized from the stream=%p length len=%lu of the wchar_array must be less than or equal to %d", stream, u_len, MAX_WCHAR_ARRAY_LENGTH);
		result = FF_FAILURE;
		goto end;
	}

	value = (wchar_t *) ff_calloc(u_len, sizeof(value[0]));
	for (i = 0; i < u_len; i++)
	{
		uint32_t ch;

		result = mrpc_uint32_unserialize(&ch, stream);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot unserialize charachter number %d from the stream=%p for wchar_array. See previous messages for more info", i, stream);
			ff_free(value);
			goto end;
		}
		if (ch >= 0x10000)
		{
			/* characters with the value higher than or equal to 0x10000
			 * should be treated as invalid, because they cannot be represented
			 * on architectures where sizeof(whcar_t) == 2.
			 */
			ff_log_debug(L"wrong character ch=%lu has been read from the stream=%p. It's value must be less than or equal to %d", ch, stream, 0x10000);
			ff_free(value);
			result = FF_FAILURE;
			goto end;
		}
		value[i] = (wchar_t) ch;
	}

	*wchar_array = mrpc_wchar_array_create(value, (int) u_len);

end:
	return result;
}

int mrpc_wchar_array_get_max_len()
{
	return MAX_WCHAR_ARRAY_LENGTH;
}
