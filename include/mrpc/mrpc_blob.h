#ifndef MRPC_BLOB_PUBLIC
#define MRPC_BLOB_PUBLIC

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
 * Creates an empty blob with the given size.
 * The returned blob is 'empty' and must be filled with contents with the given size before becoming useful.
 * Use the following template code before the blob will be useful:
 *   blob = mrpc_blob_create(blob_size);
 *   blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
 *   if (blob_stream != NULL)
 *   {
 *     write_blob_contents_into_stream(blob_stream, blob_size);
 *     ff_stream_flush(blob_stream);
 *     ff_stream_close(blob_stream);
 *     // now the blob is filled with contents and can be opened for contents reading
 *     // or moved to another file using the mrpc_blob_open_stream(blob, MRPC_BLOB_READ)
 *     // and mrpc_blob_move(blob, new_file_path);
 *   }
 * This function always returns correct result.
 */
MRPC_API struct mrpc_blob *mrpc_blob_create(int size);

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
 * Returns the size of the blob in bytes, which was passed to the mrpc_blob_create().
 */
MRPC_API int mrpc_blob_get_size(struct mrpc_blob *blob);

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
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_blob_move(struct mrpc_blob *blob, const wchar_t *new_file_path);

#ifdef __cplusplus
}
#endif

#endif
