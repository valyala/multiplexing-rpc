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
 * Returns 1 on success, 0 on error.
 */
int mrpc_blob_serialize(const struct mrpc_blob *blob, struct ff_stream *stream);

/**
 * Unserializes blob from the stream.
 * Sets the blob to newly created blob, which contains unserialized data from the stream.
 * The caller must call mrpc_blob_delete() for the returned blob.
 * Returns 1 on success, 0 on error.
 */
int mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
