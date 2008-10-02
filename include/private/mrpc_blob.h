#ifndef MRPC_BLOB_PRIVATE
#define MRPC_BLOB_PRIVATE

#include "private/mrpc_common.h"
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

struct mrpc_blob *mrpc_blob_create(int size);

void mrpc_blob_delete(struct mrpc_blob *blob);

struct ff_stream *mrpc_blob_open_stream(struct mrpc_blob *blob, enum mrpc_blob_open_stream_mode mode);

int mrpc_blob_get_size(struct mrpc_blob *blob);

#ifdef __cplusplus
}
#endif

#endif
