#include "private/mrpc_common.h"

#include "private/mrpc_bitmap.h"

struct mrpc_bitmap
{
	uint8_t *map;
	int size;
	int last_free_bit;
};

struct mrpc_bitmap *mrpc_bitmap_create(int size)
{
	struct mrpc_bitmap *bitmap;

	ff_assert(size > 0);

	bitmap = (struct mrpc_bitmap *) ff_malloc(sizeof(*bitmap));
	bitmap->map = (uint8_t *) ff_calloc(size, sizeof(bitmap->map[0]));
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
	int i;
	int n;

	ff_assert(bitmap->size > 0);
	ff_assert(bitmap->last_free_bit < bitmap->size);

	n = bitmap->last_free_bit;
	for (i = 0; i < bitmap->size; i++)
	{
		if (bitmap->map[n] == 0)
		{
			bitmap->map[n] = 1;
			bitmap->last_free_bit = n;
			break;
		}
		n++;
		if (n == bitmap->size)
		{
			n = 0;
		}
	}
	if (i == bitmap->size)
	{
		n = -1;
	}
	return n;
}

void mrpc_bitmap_release_bit(struct mrpc_bitmap *bitmap, int n)
{
	ff_assert(bitmap->size > 0);
	ff_assert(n < bitmap->size);
	ff_assert(bitmap->map[n] == 1);

	bitmap->map[n] = 0;
}
