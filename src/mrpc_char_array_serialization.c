#include "private/mrpc_common.h"

#include "private/mrpc_char_array_serialization.h"
#include "private/mrpc_char_array.h"
#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

/* the maximum char_array size.
 * This limit is security-related - it doesn't allow to send extremely long char arrays
 * in order to trigger out of memory errors.
 * The ((1 << 14) - 1) length was chosen, because it can be encoded into maximum two bytes
 * by the mrpc_uint32_serialize() function
 */
#define MAX_CHAR_ARRAY_SIZE ((1 << 14) - 1)

enum ff_result mrpc_char_array_serialize(struct mrpc_char_array *char_array, struct ff_stream *stream)
{
	int len;
	const char *value;
	enum ff_result result = FF_FAILURE;

	len = mrpc_char_array_get_len(char_array);
	ff_assert(len >= 0);
	if (len > MAX_CHAR_ARRAY_SIZE)
	{
		goto end;
	}

	result = mrpc_uint32_serialize((uint32_t) len, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}

	value = mrpc_char_array_get_value(char_array);
	result = ff_stream_write(stream, value, len);

end:
	return result;
}

enum ff_result mrpc_char_array_unserialize(struct mrpc_char_array **char_array, struct ff_stream *stream)
{
	uint32_t u_len;
	char *value;
	enum ff_result result;

	result = mrpc_uint32_unserialize(&u_len, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}
	if (u_len > MAX_CHAR_ARRAY_SIZE)
	{
		result = FF_FAILURE;
		goto end;
	}

	value = (char *) ff_calloc(u_len, sizeof(value[0]));
	result = ff_stream_read(stream, value, (int) u_len);
	if (result != FF_SUCCESS)
	{
		ff_free(value);
		goto end;
	}

	*char_array = mrpc_char_array_create(value, (int) u_len);

end:
	return result;
}
