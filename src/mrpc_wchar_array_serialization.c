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

enum ff_result mrpc_wchar_array_serialize(struct mrpc_wchar_array *wchar_array, struct ff_stream *stream)
{
	int i;
	int len;
	const wchar_t *value;
	enum ff_result result = FF_FAILURE;

	mrpc_wchar_array_inc_ref(wchar_array);
	len = mrpc_wchar_array_get_len(wchar_array);
	ff_assert(len >= 0);
	if (len > MAX_WCHAR_ARRAY_SIZE)
	{
		ff_log_warning(L"wchar_array=%p has too large len=%d for serialization. It must be less than or eqal to %d", wchar_array, len, MAX_WCHAR_ARRAY_SIZE);
		goto end;
	}

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
	if (u_len > MAX_WCHAR_ARRAY_SIZE)
	{
		ff_log_debug(L"unserialized from the stream=%p length len=%lu of the wchar_array must be less than or equal to %d", stream, u_len, MAX_WCHAR_ARRAY_SIZE);
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
