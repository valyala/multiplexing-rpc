#ifndef MRPC_CHAR_ARRAY_SERIALIZATION_PRIVATE
#define MRPC_CHAR_ARRAY_SERIALIZATION_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_char_array.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the char_array into the stream.
 * See maximum length of the char_array, which can be serialized, is in the mrcp_char_array_serialization.c file.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_char_array_serialize(struct mrpc_char_array *char_array, struct ff_stream *stream);

/**
 * Unserializes char_array from the stream into the newly allocated char_array.
 * The caller should call the mrpc_char_array_dec_ref() on the char_array when it is no longer required.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_char_array_unserialize(struct mrpc_char_array **char_array, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
