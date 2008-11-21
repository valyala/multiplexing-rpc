#ifndef MRPC_BLOB_PUBLIC_H
#define MRPC_BLOB_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mrpc_blob_open_stream_mode
{
	MRPC_BLOB_READ,
	MRPC_BLOB_WRITE
};

struct mrpc_blob;

/**
 * Creates an empty blob with the given length.
 * The returned blob is 'empty' and must be filled with contents with the given length before becoming useful.
 * Use the following template code before the blob will be useful:
 *   blob = mrpc_blob_create(blob_len);
 *   blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
 *   if (blob_stream != NULL)
 *   {
 *     write_blob_contents_into_stream(blob_stream, blob_len);
 *     ff_stream_flush(blob_stream);
 *     ff_stream_delete(blob_stream);
 *     // now the blob is filled with contents and can be opened for contents reading
 *     // or moved to another file using the mrpc_blob_open_stream(blob, MRPC_BLOB_READ)
 *     // and mrpc_blob_move(blob, new_file_path);
 *   }
 * This function always returns correct result.
 */
MRPC_API struct mrpc_blob *mrpc_blob_create(int len);

/**
 * Increments reference counter of the blob.
 */
MRPC_API void mrpc_blob_inc_ref(struct mrpc_blob *blob);

/**
 * Decrements reference counter of the blob.
 * If the reference count become zero, then the blob is automatically destroyed.
 * This function must be called the number of times which is equivalent to the number
 * of mrpc_blob_inc_ref() calls plus 1, which is paired to the mrpc_blob_create().
 */
MRPC_API void mrpc_blob_dec_ref(struct mrpc_blob *blob);

/**
 * Returns the length of the blob in bytes, which was passed to the mrpc_blob_create().
 */
MRPC_API int mrpc_blob_get_len(struct mrpc_blob *blob);

/**
 * Opens a stream for reading or writing data from / to the blob depending on the given mode.
 * The stream for writing can be opened only once just after the mrpc_blob_create() call
 * in order to fill the blob with data. Don't forget to flush the stream opened for writing before closing it:
 *   ff_flush_stream(stream_opened_for_writing);
 *   ff_close_stream(stream_opened_for_writing);
 * The stream for reading can be opened multiple times (and also simultaneously) after the blob was
 * filled by data.
 * This function returns NULL if the stream cannot be opened.
 */
MRPC_API struct ff_stream *mrpc_blob_open_stream(struct mrpc_blob *blob, enum mrpc_blob_open_stream_mode mode);

/**
 * Moves the given blob from the current path to the new_file_path path.
 * This function should be called only if the blob has a single reference,
 * and has no open streams.
 * Usually this function is called just after the blob was filled with content in order
 * to move the blob contents from temporary file into the well-known file.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_blob_move(struct mrpc_blob *blob, const wchar_t *new_file_path);

/**
 * Calculates the hash value for the given blob and the given start_value.
 * This function can be called only for the blob already filled by data.
 * Returns FF_SUCCESS on successful hash calculation, otherwise returns FF_FAILURE
 */
MRPC_API enum ff_result mrpc_blob_get_hash(struct mrpc_blob *blob, uint32_t start_value, uint32_t *hash_value);

/**
 * Serializes the blob into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_blob_serialize(struct mrpc_blob *blob, struct ff_stream *stream);

/**
 * Unserializes blob from the stream.
 * Sets the blob to newly created blob, which contains unserialized data from the stream.
 * The caller is responsible for calling the mrpc_blob_dec_ref() for the returned blob.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
