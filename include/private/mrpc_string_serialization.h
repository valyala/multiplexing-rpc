#ifndef MRPC_STRING_SERIALIZATION_PRIVATE
#define MRPC_STRING_SERIALIZATION_PRIVATE

#include "private/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the string str with the length str_len characters into the stream.
 * See maximum length of the string, which can be serialized, in the mrcp_string_serialization.c file.
 * Returns 1 on success, 0 on error.
 */
int mrpc_string_serialize(const wchar_t *str, int str_len, struct ff_stream *stream);

/**
 * Unserializes string from the stream into the newly allocated buffer pointed by str.
 * On success sets str to the newly allocated buffer, which contains the string,
 * str_len to the length of the string in characters.
 * The caller should free the str using the ff_free() call.
 * Returns 1 on success, 0 on error.
 */
int mrpc_string_unserialize(wchar_t **str, int *str_len, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
