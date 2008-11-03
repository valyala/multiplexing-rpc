#ifndef MRPC_BITMAP_PRIVATE
#define MRPC_BITMAP_PRIVATE

#include "private/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_bitmap;

/**
 * creates a bitmap
 */
struct mrpc_bitmap *mrpc_bitmap_create(int size);

/**
 * deletes the bitmap
 */
void mrpc_bitmap_delete(struct mrpc_bitmap *bitmap);

/**
 * searches for zero bit in the bitmap, sets it to 1 and returns its index.
 * Returns -1 if there are no zero bits in the bitmap.
 */
int mrpc_bitmap_acquire_bit(struct mrpc_bitmap *bitmap);

/**
 * Clears the bit number n in the bitmap from 1 to 0.
 * This bit must be previously acquired by mrpc_bitmap_acquire_bit() function.
 */
void mrpc_bitmap_release_bit(struct mrpc_bitmap *bitmap, int n);

#ifdef __cplusplus
}
#endif

#endif
