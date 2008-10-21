#include "private/mrpc_common.h"

#include "private/mrpc_string_serialization.h"
#include "private/mrpc_int_serialization.h"
#include "ff/ff_string.h"
#include "ff/ff_stream.h"

/* the maximum string size.
 * This limit is security-related - it doesn't allow to send extremely long strings
 * in order to trigger out of memory errors.
 * The ((1 << 14) - 1) length was chosen, because it can be encoded into maximum two bytes
 * by the mrpc_uint32_serialize() function
 */
#define MAX_STRING_SIZE ((1 << 14) - 1)

enum ff_result mrpc_string_serialize(const ff_string *str, struct ff_stream *stream)
{
	int i;
	int str_len;
	const wchar_t *str_cstr;
	enum ff_result result = FF_FAILURE;

	str_len = ff_string_get_length(str);
	ff_assert(str_len >= 0);
	if (str_len > MAX_STRING_SIZE)
	{
		goto end;
	}

	result = mrpc_uint32_serialize((uint32_t) str_len, stream);
	if (result == FF_FAILURE)
	{
		goto end;
	}

	str_cstr = ff_string_get_cstr(str);
	for (i = 0; i < str_len; i++)
	{
		uint32_t ch;
		int len;

		ch = (uint32_t) str[i];
		result = mrpc_uint32_serialize(ch, stream);
		if (result == FF_FAILURE)
		{
			goto end;
		}
	}

end:
	return result;
}

enum ff_result mrpc_string_unserialize(ff_string **str, struct ff_stream *stream)
{
	uint32_t i;
	uint32_t u_str_len;
	uint32_t str_size;
	wchar_t *str_cstr;
	enum ff_result result;

	result = mrpc_uint32_unserialize(&u_str_len, stream);
	if (result == FF_FAILURE)
	{
		goto end;
	}
	if (u_str_len > MAX_STRING_SIZE)
	{
		result = FF_FAILURE;
		goto end;
	}

	/* integer overflow is impossible here, because MAX_STRING_SIZE and, consequently,
	 * u_str_len is guaranteed to be less than MAX_INT / sizeof(wchar_t)
	 */
	ff_assert(MAX_INT / sizeof(wchar_t) > MAX_STRING_SIZE);
	str_size = sizeof(wchar_t) * u_str_len;
	str_cstr = (wchar_t *) ff_malloc(str_size);

	for (i = 0; i < u_str_len; i++)
	{
		uint32_t ch;

		result = mrpc_uint32_unserialize(&ch, stream);
		if (result == FF_FAILURE)
		{
			ff_free(str_cstr);
			goto end;
		}
		if (ch > WCHAR_MAX)
		{
			ff_free(str_cstr);
			result = FF_FAILURE;
			goto end;
		}
		str_cstr[i] = (wchar_t) ch;
	}

	*str = ff_string_create(str_cstr, (int) u_str_len);

end:
	return result;
}
