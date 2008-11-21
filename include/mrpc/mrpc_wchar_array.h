#ifndef MRPC_WCHAR_ARRAY_PUBLIC_H
#define MRPC_WCHAR_ARRAY_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_wchar_array;

/**
 * creates the wchar array structure from the value. Len is the length of the value wchar array.
 * It is expected that the len is less than or equal to the value returned from the mrpc_wchar_array_get_max_len().
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

/**
 * Serializes the wchar_array into the stream.
 * See maximum length of the wchar_array, which can be serialized, is in the mrcp_wchar_array_serialization.c file.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_wchar_array_serialize(struct mrpc_wchar_array *wchar_array, struct ff_stream *stream);

/**
 * Unserializes wchar_array from the stream into the newly allocated wchar_array.
 * The caller should call the mrpc_wchar_array_dec_ref() on the wchar_array when it is no longer required.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_wchar_array_unserialize(struct mrpc_wchar_array **wchar_array, struct ff_stream *stream);

/**
 * returns the maximum length of the wchar array, which can be created using the mrpc_wchar_array_create()
 */
MRPC_API int mrpc_wchar_array_get_max_len();

#ifdef __cplusplus
}
#endif

#endif
