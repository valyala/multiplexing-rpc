#include "private/mrpc_common.h"

#include "private/mrpc_wchar_array_serialization.h"
#include "private/mrpc_wchar_array.h"
#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

/* the maximum wchar_array size.
 * This limit is security-related - it doesn't allow to send extremely long wchar arrays
 * in order to trigger out of memory errors.
 * The ((1 << 14) - 1) length was chosen, because it can be encoded into maximum two bytes
 * by the mrpc_uint32_serialize() function
 */
#define MAX_WCHAR_ARRAY_SIZE ((1 << 14) - 1)

enum ff_result mrpc_wchar_array_serialize(const struct mrpc_wchar_array *wchar_array, struct ff_stream *stream)
{
	int i;
	int len;
	const wchar_t *value;
	enum ff_result result = FF_FAILURE;

	len = mrpc_wchar_array_get_len(wchar_array);
	ff_assert(len >= 0);
	if (len > MAX_WCHAR_ARRAY_SIZE)
	{
		goto end;
	}

	result = mrpc_uint32_serialize((uint32_t) len, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}

	value = mrpc_wchar_array_get_value(wchar_array);
	for (i = 0; i < len; i++)
	{
		uint32_t ch;
		int len;

		ch = (uint32_t) value[i];
		result = mrpc_uint32_serialize(ch, stream);
		if (result != FF_SUCCESS)
		{
			goto end;
		}
	}

end:
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
		goto end;
	}
	if (u_len > MAX_WCHAR_ARRAY_SIZE)
	{
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
			ff_free(value);
			goto end;
		}
		if (ch > WCHAR_MAX)
		{
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
