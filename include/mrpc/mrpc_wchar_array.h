#ifndef MRPC_WCHAR_ARRAY_PUBLIC
#define MRPC_WCHAR_ARRAY_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_wchar_array;

/**
 * creates the wchar array structure from the value. Len is the length of the value wchar array.
 * This function acquires ownership of the value, so the caller shouldn't free the memory
 * allocated for the value.
 * Also this means that the value must be allocated only by ff_malloc() or ff_calloc() functions!
 * Always returns correct result.
 */
MRPC_API struct mrpc_wchar_array *mrpc_wchar_array_create(const wchar_t *value, int len);

/**
 * increments reference counter for the wchar_array.
 */
MRPC_API void mrpc_wchar_array_inc_ref(struct mrpc_wchar_array *wchar_array);

/**
 * decrements reference counter for the wchar_array.
 * When reference counter reaches zero, then the wchar_array is completely destroyed.
 */
MRPC_API void mrpc_wchar_array_dec_ref(struct mrpc_wchar_array *wchar_array);

/**
 * returns the pointer to the value passed to the mrpc_wchar_array_create().
 */
MRPC_API const wchar_t *mrpc_wchar_array_get_value(struct mrpc_wchar_array *wchar_array);

/**
 * Returns the length of the wchar_array passed to the mrpc_wchar_array_create().
 */
MRPC_API int mrpc_wchar_array_get_len(struct mrpc_wchar_array *wchar_array);

/**
 * Returns the hash value for the given wchar_array and the given start_value.
 */
MRPC_API uint32_t mrpc_wchar_array_get_hash(struct mrpc_wchar_array *wchar_array, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
