#ifndef MRPC_BITMAP_PRIVATE
#define MRPC_BITMAP_PRIVATE

#include "private/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_bitmap;

struct mrpc_bitmap *mrpc_bitmap_create(int size);

void mrpc_bitmap_delete(struct mrpc_bitmap *bitmap);

int mrpc_bitmap_acquire_bit(struct mrpc_bitmap *bitmap);

void mrpc_bitmap_release_bit(struct mrpc_bitmap *bitmap, int n);

#ifdef __cplusplus
}
#endif

#endif
