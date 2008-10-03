#ifndef MRPC_BLOB_PRIVATE
#define MRPC_BLOB_PRIVATE

#include "private/mrpc_common.h"
#include "ff/ff_stream.h"
#include "ff/ff_string.h"

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
 * It returns NULL if the blob cannot be created.
 */
struct mrpc_blob *mrpc_blob_create(int size);

/**
 * Increments reference count to the blob.
 */
void mrpc_blob_inc_ref(struct mrpc_blob *blob);

/**
 * Decrements reference count to the blob.
 * If the reference count become zero, then the blob is automatically destroyed.
 */
void mrpc_blob_dec_ref(struct mrpc_blob *blob);

/**
 * Returns the size of the blob.
 */
int mrpc_blob_get_size(struct mrpc_blob *blob);

/**
 * Opens a stream for reading or writing data from / to the blob.
 * Returns NULL if the stream cannot be opened.
 */
struct ff_stream *mrpc_blob_open_stream(struct mrpc_blob *blob, enum mrpc_blob_open_stream_mode mode);

/**
 * Moves the given blob from the current path to the new_file_path path.
 * This function should be called only if the blob has a single reference,
 * and no open streams.
 */
int mrpc_blob_move(struct mrpc_blob *blob, struct ff_string *new_file_path);

#ifdef __cplusplus
}
#endif

#endif
