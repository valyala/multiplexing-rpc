#include "private/mrpc_common.h"

#include "private/mrpc_blob_serialization.h"
#include "private/mrpc_int_serialization.h"
#include "private/mrpc_blob.h"
#include "ff/ff_stream.h"

int mrpc_blob_serialize(const struct mrpc_blob *blob, struct ff_stream *stream)
{
	int is_success = 0;
	int blob_len;
	struct ff_stream *blob_stream;

	blob_len = mrpc_blob_get_size(blob);
	ff_assert(blob_len >= 0);
	is_success = mrpc_uint32_serialize((uint32_t) blob_len, stream);
	if (!is_success)
	{
		goto end;
	}

	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	if (blob_stream == NULL)
	{
		/* error when opening blob stream */
		is_success = 0;
		goto end;
	}
	is_success = ff_stream_copy(blob_stream, stream, blob_len);
	ff_stream_close(blob_stream);

end:
	return is_success;
}

int mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream)
{
	int is_success;
	int blob_len;
	struct blob *new_blob;
	struct ff_stream *blob_stream;

	is_success = mrpc_uint32_unserialize((uint32_t *) &blob_len, stream);
	if (!is_success)
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
		/* error when opening blob stream */
		mrpc_blob_dec_ref(new_blob);
		is_success = 0;
		goto end;
	}
	is_success = ff_stream_copy(stream, blob_stream, blob_len);
	is_success = ff_stream_flush(blob_stream);
	if (!is_success)
	{
		mrpc_blob_dec_ref(new_blob);
		goto end;
	}
	ff_stream_close(blob_stream);
	if (!is_success)
	{
		mrpc_blob_dec_ref(new_blob);
		goto end;
	}
	*blob = new_blob;

end:
	return is_success;
}
