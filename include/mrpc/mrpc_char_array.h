#ifndef MRPC_CHAR_ARRAY_PUBLIC
#define MRPC_CHAR_ARRAY_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_char_array;

/**
 * creates the char array structure from the value. Len is the length of the value char array.
 * This function acquires ownership of the value, so the caller shouldn't free the memory
 * allocated for the value.
 * Also this means that the value must be allocated only by ff_malloc() or ff_calloc() functions!
 * Always returns correct result.
 */
MRPC_API struct mrpc_char_array *mrpc_char_array_create(const char *value, int len);

/**
 * increments reference counter for the char_array.
 */
MRPC_API void mrpc_char_array_inc_ref(struct mrpc_char_array *char_array);

/**
 * decrements reference counter for the char_array.
 * When reference counter reaches zero, then the char_array is completely destroyed.
 */
MRPC_API void mrpc_char_array_dec_ref(struct mrpc_char_array *char_array);

/**
 * returns the pointer to the value passed to the mrpc_char_array_create().
 */
MRPC_API const char *mrpc_char_array_get_value(struct mrpc_char_array *char_array);

/**
 * Returns the length of the char_array passed to the mrpc_char_array_create().
 */
MRPC_API int mrpc_char_array_get_len(struct mrpc_char_array *char_array);

/**
 * Returns the hash value for the given char_array and the given start_value.
 */
MRPC_API uint32_t mrpc_char_array_get_hash(struct mrpc_char_array *char_array, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
