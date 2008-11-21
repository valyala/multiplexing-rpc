#include "private/mrpc_common.h"

#include "private/mrpc_char_array.h"
#include "private/mrpc_int.h"
#include "ff/ff_hash.h"

/* the maximum char_array length.
 * This limit is security-related - it doesn't allow to send extremely long char arrays
 * in order to trigger out of memory errors.
 * The ((1 << 14) - 1) length was chosen, because it can be encoded into maximum two bytes
 * by the mrpc_uint32_serialize() function
 */
#define MAX_CHAR_ARRAY_LENGTH ((1 << 14) - 1)

struct mrpc_char_array
{
	const char *value;
	int len;
	int ref_cnt;
};

static struct mrpc_char_array *create_char_array(const char *value, int len)
{
	struct mrpc_char_array *char_array;

	ff_assert(len >= 0);
	ff_assert(len <= MAX_CHAR_ARRAY_LENGTH);
	ff_assert(value != NULL);

	char_array = (struct mrpc_char_array *) ff_malloc(sizeof(*char_array));
	char_array->value = value;
	char_array->len = len;
	char_array->ref_cnt = 1;

	return char_array;
}

static void delete_char_array(struct mrpc_char_array *char_array)
{
	ff_assert(char_array->ref_cnt == 0);
	ff_assert(char_array->value != NULL);

	ff_free((void *) char_array->value);
	ff_free(char_array);
}

struct mrpc_char_array *mrpc_char_array_create(const char *value, int len)
{
	struct mrpc_char_array *char_array;

	ff_assert(len >= 0);
	ff_assert(len <= MAX_CHAR_ARRAY_LENGTH);
	ff_assert(value != NULL);

	char_array = create_char_array(value, len);
	return char_array;
}

void mrpc_char_array_inc_ref(struct mrpc_char_array *char_array)
{
	char_array->ref_cnt++;
	ff_assert(char_array->ref_cnt > 1);
}

void mrpc_char_array_dec_ref(struct mrpc_char_array *char_array)
{
	ff_assert(char_array->ref_cnt > 0);
	char_array->ref_cnt--;
	if (char_array->ref_cnt == 0)
	{
		delete_char_array(char_array);
	}
}

const char *mrpc_char_array_get_value(struct mrpc_char_array *char_array)
{
	ff_assert(char_array->ref_cnt > 0);
	ff_assert(char_array->value != NULL);

	return char_array->value;
}

int mrpc_char_array_get_len(struct mrpc_char_array *char_array)
{
	ff_assert(char_array->ref_cnt > 0);
	ff_assert(char_array->len >= 0);
	ff_assert(char_array->len <= MAX_CHAR_ARRAY_LENGTH);

	return char_array->len;
}

uint32_t mrpc_char_array_get_hash(struct mrpc_char_array *char_array, uint32_t start_value)
{
	const char *value;
	uint32_t hash_value;
	int len;

	ff_assert(char_array->ref_cnt > 0);

	value = mrpc_char_array_get_value(char_array);
	len = mrpc_char_array_get_len(char_array);
	hash_value = ff_hash_uint8(start_value, (uint8_t *) value, len);
	return hash_value;
}

enum ff_result mrpc_char_array_serialize(struct mrpc_char_array *char_array, struct ff_stream *stream)
{
	int len;
	const char *value;
	enum ff_result result = FF_FAILURE;

	mrpc_char_array_inc_ref(char_array);
	len = mrpc_char_array_get_len(char_array);

	result = mrpc_uint32_serialize((uint32_t) len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot serialize char_array=%p length len=%d into the stream=%p. See previous messages for more info", char_array, len, stream);
		goto end;
	}

	value = mrpc_char_array_get_value(char_array);
	result = ff_stream_write(stream, value, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write char_array=%p contents with len=%p into the stream=%p. See previous messages for more info", char_array, len, stream);
	}

end:
	mrpc_char_array_dec_ref(char_array);
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
		ff_log_debug(L"cannot unserialize char_array length from the stream=%p. See previous messages for more info", stream);
		goto end;
	}
	if (u_len > MAX_CHAR_ARRAY_LENGTH)
	{
		ff_log_debug(L"unserialized from the stream=%p length len=%lu of the char_array must be less than or equal to %d", stream, u_len, MAX_CHAR_ARRAY_LENGTH);
		result = FF_FAILURE;
		goto end;
	}

	value = (char *) ff_calloc(u_len, sizeof(value[0]));
	result = ff_stream_read(stream, value, (int) u_len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot read char_array contents with length len=%lu from the stream=%p. See prevsious messages for more info", u_len, stream);
		ff_free(value);
		goto end;
	}

	*char_array = mrpc_char_array_create(value, (int) u_len);

end:
	return result;
}

int mrpc_char_array_get_max_len()
{
	return MAX_CHAR_ARRAY_LENGTH;
}
