#include "private/mrpc_common.h"

#include "private/mrpc_blob_serialization.h"
#include "private/mrpc_int_serialization.h"
#include "private/mrpc_blob.h"
#include "ff/ff_stream.h"

enum ff_result mrpc_blob_serialize(struct mrpc_blob *blob, struct ff_stream *stream)
{
	int blob_len;
	struct ff_stream *blob_stream;
	enum ff_result result = FF_FAILURE;

	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	if (blob_stream == NULL)
	{
		ff_log_debug(L"cannot open blob stream for reading for the blob=%p. See previous messages for more info", blob);
		goto end;
	}

	blob_len = mrpc_blob_get_size(blob);
	ff_assert(blob_len >= 0);
	result = mrpc_uint32_serialize((uint32_t) blob_len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"error occured while serializing blob_len=%d into the stream=%p. See previous messages for more info", blob_len, stream);
		ff_stream_delete(blob_stream);
		goto end;
	}

	result = ff_stream_copy(blob_stream, stream, blob_len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"error occured while copying blob_stream=%p contents to the stream=%p, blob_len=%d. See previous messages for more info", blob_stream, stream, blob_len);
	}
	ff_stream_delete(blob_stream);

end:
	return result;
}

enum ff_result mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream)
{
	int blob_len;
	struct mrpc_blob *new_blob;
	struct ff_stream *blob_stream;
	enum ff_result result;

	result = mrpc_uint32_unserialize((uint32_t *) &blob_len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"error occured while unserializing blob length form the stream=%p. See previous messages for more info", stream);
		goto end;
	}
	if (blob_len < 0)
	{
		ff_log_debug(L"unexpected blob_len=%d value read from the stream=%p. It must be posisitive", blob_len, stream);
		goto end;
	}

	new_blob = mrpc_blob_create(blob_len);
	blob_stream = mrpc_blob_open_stream(new_blob, MRPC_BLOB_WRITE);
	if (blob_stream == NULL)
	{
		ff_log_debug(L"cannot open blob stream for writing to the new_blob=%p, blob_len=%d. See previous messages for more info", new_blob, blob_len);
		mrpc_blob_dec_ref(new_blob);
		result = FF_FAILURE;
		goto end;
	}
	result = ff_stream_copy(stream, blob_stream, blob_len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot copy stream=%p to the blob_stream=%p, blob_len=%d. See previous messages for more info", stream, blob_stream, blob_len);
		ff_stream_delete(blob_stream);
		mrpc_blob_dec_ref(new_blob);
		goto end;
	}
	result = ff_stream_flush(blob_stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot flus the blob_stream=%p. See previous messages for more info", blob_stream);
		ff_stream_delete(blob_stream);
		mrpc_blob_dec_ref(new_blob);
		goto end;
	}
	ff_stream_delete(blob_stream);
	*blob = new_blob;

end:
	return result;
}
