#ifndef MRPC_WCHAR_ARRAY_SERIALIZATION_PRIVATE
#define MRPC_WCHAR_ARRAY_SERIALIZATION_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_wchar_array.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the wchar_array into the stream.
 * See maximum length of the wchar_array, which can be serialized, is in the mrcp_wchar_array_serialization.c file.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_wchar_array_serialize(const struct mrpc_wchar_array *wchar_array, struct ff_stream *stream);

/**
 * Unserializes wchar_array from the stream into the newly allocated wchar_array.
 * The caller should call the mrpc_wchar_array_dec_ref() on the wchar_array when it is no longer required.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_wchar_array_unserialize(struct mrpc_wchar_array **wchar_array, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
