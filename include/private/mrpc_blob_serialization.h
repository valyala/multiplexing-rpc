#ifndef MRPC_BLOB_SERIALIZATION_PRIVATE
#define MRPC_BLOB_SERIALIZATION_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_blob.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the blob into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_blob_serialize(struct mrpc_blob *blob, struct ff_stream *stream);

/**
 * Unserializes blob from the stream.
 * Sets the blob to newly created blob, which contains unserialized data from the stream.
 * The caller is responsible for calling the mrpc_blob_dec_ref() for the returned blob.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
