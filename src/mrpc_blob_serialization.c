#include "private/mrpc_common.h"

#include "private/mrpc_blob_serialization.h"
#include "private/mrpc_int_serialization.h"
#include "private/mrpc_blob.h"
#include "ff/ff_stream.h"

enum ff_result mrpc_blob_serialize(const struct mrpc_blob *blob, struct ff_stream *stream)
{
	int blob_len;
	struct ff_stream *blob_stream;
	enum ff_result result = FF_FAILURE;

	blob_len = mrpc_blob_get_size(blob);
	ff_assert(blob_len >= 0);
	result = mrpc_uint32_serialize((uint32_t) blob_len, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}

	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	if (blob_stream == NULL)
	{
		result = FF_FAILURE;
		goto end;
	}
	result = ff_stream_copy(blob_stream, stream, blob_len);
	ff_stream_delete(blob_stream);

end:
	return result;
}

enum ff_result mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream)
{
	int blob_len;
	struct blob *new_blob;
	struct ff_stream *blob_stream;
	enum ff_result result;

	result = mrpc_uint32_unserialize((uint32_t *) &blob_len, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}
	if (blob_len < 0)
	{
		goto end;
	}

	new_blob = mrpc_blob_create(blob_len);
	blob_stream = mrpc_blob_open_stream(new_blob, MRPC_BLOB_WRITE);
	if (blob_stream == NULL)
	{
		mrpc_blob_dec_ref(new_blob);
		result = FF_FAILURE;
		goto end;
	}
	result = ff_stream_copy(stream, blob_stream, blob_len);
	if (result != FF_SUCCESS)
	{
		ff_stream_delete(blob_stream);
		mrpc_blob_dec_ref(new_blob);
		goto end;
	}
	result = ff_stream_flush(blob_stream);
	if (result != FF_SUCCESS)
	{
		ff_stream_delete(blob_stream);
		mrpc_blob_dec_ref(new_blob);
		goto end;
	}
	ff_stream_delete(blob_stream);
	*blob = new_blob;

end:
	return result;
}
