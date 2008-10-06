#ifndef MRPC_STRING_SERIALIZATION_PRIVATE
#define MRPC_STRING_SERIALIZATION_PRIVATE

#include "private/mrpc_common.h"
#include "ff/ff_stream.h"
#include "ff/ff_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the string str into the stream.
 * See maximum length of the string, which can be serialized, in the mrcp_string_serialization.c file.
 * Returns 1 on success, 0 on error.
 */
int mrpc_string_serialize(const ff_string *str, struct ff_stream *stream);

/**
 * Unserializes string from the stream into the newly allocated str.
 * The caller should call the ff_string_dec_ref() on the str when it is no longer required.
 * Returns 1 on success, 0 on error.
 */
int mrpc_string_unserialize(ff_string **str, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
