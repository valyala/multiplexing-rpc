#include "private/mrpc_common.h"

#include "private/mrpc_bitmap.h"

#define GET_BIT_U64(u64, bit_num) (((u64) >> (bit_num)) & 1ull)
#define SET_BIT_U64(u64, bit_num) (u64) |= 1ull << (bit_num)
#define CLEAR_BIT_U64(u64, bit_num) (u64) &= ~(1ull << (bit_num))
#define U64_ARRAY_INDEX(bit_num) ((bit_num) >> 6)
#define U64_BIT_NUM(bit_num) ((bit_num) & 0x3f)

#define GET_BIT(u64_array, bit_num) GET_BIT_U64((u64_array)[U64_ARRAY_INDEX(bit_num)], U64_BIT_NUM(bit_num))
#define SET_BIT(u64_array, bit_num) SET_BIT_U64((u64_array)[U64_ARRAY_INDEX(bit_num)], U64_BIT_NUM(bit_num))
#define CLEAR_BIT(u64_array, bit_num) CLEAR_BIT_U64((u64_array)[U64_ARRAY_INDEX(bit_num)], U64_BIT_NUM(bit_num))

#define GET_U64_ARRAY_SIZE(bits_size) (((bits_size) + 0x3f) >> 6)

struct mrpc_bitmap
{
	uint64_t *map;
	int size;
	int last_free_bit;
};

struct mrpc_bitmap *mrpc_bitmap_create(int size)
{
	struct mrpc_bitmap *bitmap;
	int map_size;

	ff_assert(size > 0);
	map_size = GET_U64_ARRAY_SIZE(size);

	bitmap = (struct mrpc_bitmap *) ff_malloc(sizeof(*bitmap));
	bitmap->map = (uint64_t *) ff_calloc(map_size, sizeof(bitmap->map[0]));
	bitmap->size = size;
	bitmap->last_free_bit = 0;
	return bitmap;
}

void mrpc_bitmap_delete(struct mrpc_bitmap *bitmap)
{
	ff_free(bitmap->map);
	ff_free(bitmap);
}

int mrpc_bitmap_acquire_bit(struct mrpc_bitmap *bitmap)
{
	uint64_t *map;
	int i;
	int n;
	int bitmap_size;

	ff_assert(bitmap->size > 0);
	ff_assert(bitmap->last_free_bit < bitmap->size);

	map = bitmap->map;
	n = bitmap->last_free_bit;
	bitmap_size = bitmap->size;
	for (i = 0; i < bitmap_size; i++)
	{
		if (GET_BIT(map, n) == 0)
		{
			SET_BIT(map, n);
			bitmap->last_free_bit = n;
			break;
		}
		n++;
		if (n == bitmap_size)
		{
			n = 0;
		}
	}
	if (i == bitmap_size)
	{
		ff_log_debug(L"all bits are occupied in the bitmap=%p", bitmap);
		n = -1;
	}
	return n;
}

void mrpc_bitmap_release_bit(struct mrpc_bitmap *bitmap, int n)
{
	ff_assert(bitmap->size > 0);
	ff_assert(n < bitmap->size);
	ff_assert(GET_BIT(bitmap->map, n) == 1);

	CLEAR_BIT(bitmap->map, n);
}

