#include "private/mrpc_common.h"

#include "private/mrpc_char_array.h"
#include "ff/ff_hash.h"

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
	ff_assert(value != NULL);

	char_array = (struct mrpc_char_array *) ff_malloc(sizeof(*char_array));
	char_array->value = value;
	char_array->len = len;
	char_array->ref_cnt = 1;

	return char_array;
}

static void delete_char_array(struct mrpc_char_array *char_array)
{
	ff_free(char_array->value);
	ff_free(char_array);
}

struct mrpc_char_array *mrpc_char_array_create(const char *value, int len)
{
	struct mrpc_char_array *char_array;

	ff_assert(len >= 0);
	ff_assert(value != NULL);

	char_array = create_char_array(value, len);
	return char_array;
}

void mrpc_char_array_inc_ref(struct mrpc_char_array *char_array)
{
	char_array->ref_cnt++;
	ff_assert(char_array->ref_cnt > 0);
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
	return char_array->value;
}

int mrpc_char_array_get_len(struct mrpc_char_array *char_array)
{
	return char_array->len;
}

uint32_t mrpc_char_array_get_hash(struct mrpc_char_array *char_array, uint32_t start_value)
{
	uint32_t hash_value;

	hash_vlaue = ff_hash_uint8(start_value, char_array->value, char_array->len);
	return hash_value;
}
