#ifndef MRPC_CHAR_ARRAY_PUBLIC_H
#define MRPC_CHAR_ARRAY_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_char_array;

/**
 * creates the char array structure from the value. Len is the length of the value char array.
 * It is expected that the len is less than or equal to the value returned from the mrpc_char_array_get_max_len().
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

/**
 * Serializes the char_array into the stream.
 * See maximum length of the char_array, which can be serialized, is in the mrcp_char_array_serialization.c file.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_char_array_serialize(struct mrpc_char_array *char_array, struct ff_stream *stream);

/**
 * Unserializes char_array from the stream into the newly allocated char_array.
 * The caller should call the mrpc_char_array_dec_ref() on the char_array when it is no longer required.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_char_array_unserialize(struct mrpc_char_array **char_array, struct ff_stream *stream);

/**
 * returns the maximum length of the char array, which can be created using the mrpc_char_array_create()
 */
MRPC_API int mrpc_char_array_get_max_len();

#ifdef __cplusplus
}
#endif

#endif
