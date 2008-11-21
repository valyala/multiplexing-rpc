#include "private/mrpc_common.h"

#include "private/mrpc_blob.h"
#include "private/mrpc_int.h"
#include "ff/ff_stream.h"
#include "ff/ff_file.h"
#include "ff/arch/ff_arch_misc.h"

/**
 * when the blob is created using the following sequence:
 *
 *   blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
 *   write_blob_contents_to_the_stream(blob_stream);
 *   ff_stream_flush(blob_stream);
 *   ff_stream_delete(blob_stream);
 *
 * then the blob content is written into separate unique file in the temporary directory.
 * The unique file name will start with the BLOB_FILENAME_PREFIX.
 */
#define BLOB_FILENAME_PREFIX L"mrpc_blob."
#define BLOB_FILENAME_PREFIX_LEN ((sizeof(BLOB_FILENAME_PREFIX) / sizeof(BLOB_FILENAME_PREFIX[0])) - 1)

enum blob_state
{
	BLOB_EMPTY,
	BLOB_INCOMPLETE,
	BLOB_COMPLETE
};

struct mrpc_blob
{
	const wchar_t *file_path;
	int len;
	enum blob_state state;
	int ref_cnt;
};

struct blob_stream_data
{
	struct mrpc_blob *blob;
	struct ff_file *file;
	enum mrpc_blob_open_stream_mode mode;
	int curr_pos;
};

static void delete_blob_stream(struct ff_stream *stream)
{
	struct blob_stream_data *data;

	data = (struct blob_stream_data *) ff_stream_get_ctx(stream);

	ff_file_close(data->file);
	mrpc_blob_dec_ref(data->blob);
	ff_free(data);
}

static enum ff_result read_from_blob_stream(struct ff_stream *stream, void *buf, int len)
{
	struct blob_stream_data *data;
	int bytes_left;
	enum ff_result result = FF_FAILURE;

	ff_assert(len >= 0);

	data = (struct blob_stream_data *) ff_stream_get_ctx(stream);

	ff_assert(data->mode == MRPC_BLOB_READ);
	ff_assert(data->blob->state == BLOB_COMPLETE);
	ff_assert(data->curr_pos >= 0);
	ff_assert(data->curr_pos <= data->blob->len);
	bytes_left = data->blob->len - data->curr_pos;
	if (len <= bytes_left)
	{
		result = ff_file_read(data->file, buf, len);
		if (result == FF_SUCCESS)
		{
			data->curr_pos += len;
		}
		else
		{
			ff_log_debug(L"error while reading from the blob stream=%p to the buf=%p, len=%d. See previous messages for more info", stream, buf, len);
		}
	}
	else
	{
		ff_log_debug(L"cannot read len=%d bytes from the blob stream=%p, because only %d bytes left", len, stream, bytes_left);
	}
	return result;
}

static enum ff_result write_to_blob_stream(struct ff_stream *stream, const void *buf, int len)
{
	struct blob_stream_data *data;
	int bytes_left;
	enum ff_result result = FF_FAILURE;

	ff_assert(len >= 0);

	data = (struct blob_stream_data *) ff_stream_get_ctx(stream);

	ff_assert(data->mode = MRPC_BLOB_WRITE);
	ff_assert(data->blob->state != BLOB_EMPTY);
	ff_assert(data->curr_pos >= 0);
	ff_assert(data->curr_pos <= data->blob->len);
	bytes_left = data->blob->len - data->curr_pos;
	if (len <= bytes_left)
	{
		result = ff_file_write(data->file, buf, len);
		if (result == FF_SUCCESS)
		{
			data->curr_pos += len;
		}
		else
		{
			ff_log_debug(L"error while writing to the blob stream=%p from the buf=%p, len=%d. See previous messages for more info", stream, buf, len);
		}
	}
	else
	{
		ff_log_debug(L"cannot write len=%d bytes to the stream=%p, because only %d bytes can be written", len, stream, bytes_left);
	}
	return result;
}

static enum ff_result flush_blob_stream(struct ff_stream *stream)
{
	struct blob_stream_data *data;
	enum ff_result result;

	data = (struct blob_stream_data *) ff_stream_get_ctx(stream);

	ff_assert(data->mode == MRPC_BLOB_WRITE);
	ff_assert(data->blob->state != BLOB_EMPTY);
	result = ff_file_flush(data->file);
	if (result == FF_SUCCESS)
	{
		if (data->curr_pos == data->blob->len)
		{
			data->blob->state = BLOB_COMPLETE;
		}
	}
	else
	{
		ff_log_debug(L"error while flushing the blob stream=%p. See previous messages for more info", stream);
	}
	return result;
}

static void disconnect_blob_stream(struct ff_stream *stream)
{
	/* this operation doesn't supported by the blob_stream */
	ff_assert(0);
}

static const struct ff_stream_vtable blob_stream_vtable =
{
	delete_blob_stream,
	read_from_blob_stream,
	write_to_blob_stream,
	flush_blob_stream,
	disconnect_blob_stream
};

static const wchar_t *create_temporary_file_path()
{
	const wchar_t *tmp_dir_path;
	const wchar_t *file_path;
	wchar_t *tmp_file_path;
	int tmp_dir_path_len;
	int file_path_len;

	ff_arch_misc_get_tmp_dir_path(&tmp_dir_path, &tmp_dir_path_len);
	ff_assert(tmp_dir_path != NULL);
	ff_assert(tmp_dir_path[tmp_dir_path_len] == 0);
	ff_assert(tmp_dir_path[tmp_dir_path_len - 1] == L'/' || tmp_dir_path[tmp_dir_path_len - 1] == L'\\');
	ff_arch_misc_create_unique_file_path(tmp_dir_path, tmp_dir_path_len,
		BLOB_FILENAME_PREFIX, BLOB_FILENAME_PREFIX_LEN, &file_path, &file_path_len);
	ff_assert(file_path != NULL);

	tmp_file_path = (wchar_t *) ff_calloc(file_path_len + 1, sizeof(tmp_file_path[0]));
	memcpy(tmp_file_path, file_path, file_path_len * sizeof(file_path[0]));
	ff_arch_misc_delete_unique_file_path(file_path);

	return tmp_file_path;
}

static struct mrpc_blob *create_blob(int len)
{
	struct mrpc_blob *blob;

	ff_assert(len >= 0);

	blob = (struct mrpc_blob *) ff_malloc(sizeof(*blob));
	blob->file_path = create_temporary_file_path();
	blob->len = len;
	blob->state = BLOB_EMPTY;
	blob->ref_cnt = 1;
	return blob;
}

static void delete_blob(struct mrpc_blob *blob)
{
	ff_assert(blob->ref_cnt == 0);

	if (blob->state != BLOB_EMPTY)
	{
		enum ff_result result;

		result = ff_file_erase(blob->file_path);
		if (result != FF_SUCCESS)
		{
			ff_log_warning(L"cannot delete the blob backing file [%ls]", blob->file_path);
		}
	}

	ff_free((void *) blob->file_path);
	ff_free(blob);
}

struct mrpc_blob *mrpc_blob_create(int len)
{
	struct mrpc_blob *blob;

	ff_assert(len >= 0);

	blob = create_blob(len);
	return blob;
}

void mrpc_blob_inc_ref(struct mrpc_blob *blob)
{
	blob->ref_cnt++;
	ff_assert(blob->ref_cnt > 1);
}

void mrpc_blob_dec_ref(struct mrpc_blob *blob)
{
	ff_assert(blob->ref_cnt > 0);
	blob->ref_cnt--;
	if (blob->ref_cnt == 0)
	{
		delete_blob(blob);
	}
}

int mrpc_blob_get_len(struct mrpc_blob *blob)
{
	ff_assert(blob->ref_cnt > 0);
	ff_assert(blob->len >= 0);

	return blob->len;
}

struct ff_stream *mrpc_blob_open_stream(struct mrpc_blob *blob, enum mrpc_blob_open_stream_mode mode)
{
	struct ff_stream *stream = NULL;
	struct blob_stream_data *data;
	struct ff_file *file;

	ff_assert(blob->ref_cnt > 0);

	if (mode == MRPC_BLOB_READ)
	{
		ff_assert(blob->state == BLOB_COMPLETE);
		mrpc_blob_inc_ref(blob);
		file = ff_file_open(blob->file_path, FF_FILE_READ);
		if (file == NULL)
		{
			ff_log_warning(L"cannot open the blob backing file [%ls] for reading", blob->file_path);
			mrpc_blob_dec_ref(blob);
			goto end;
		}
	}
	else
	{
		ff_assert(mode == MRPC_BLOB_WRITE);
		ff_assert(blob->ref_cnt == 1);
		ff_assert(blob->state == BLOB_EMPTY);
		mrpc_blob_inc_ref(blob);
		file = ff_file_open(blob->file_path, FF_FILE_WRITE);
		if (file == NULL)
		{
			ff_log_warning(L"cannot open the blob backing file [%ls] for writing", blob->file_path);
			mrpc_blob_dec_ref(blob);
			goto end;
		}
		blob->state = BLOB_INCOMPLETE;
	}

	data = (struct blob_stream_data *) ff_malloc(sizeof(*data));
	data->blob = blob;
	data->file = file;
	data->mode = mode;
	data->curr_pos = 0;

	stream = ff_stream_create(&blob_stream_vtable, data);

end:
	return stream;
}

enum ff_result mrpc_blob_move(struct mrpc_blob *blob, const wchar_t *new_file_path)
{
	enum ff_result result;

	ff_assert(blob->state == BLOB_COMPLETE);
	ff_assert(blob->ref_cnt == 1);

	mrpc_blob_inc_ref(blob);
	result = ff_file_move(blob->file_path, new_file_path);
	if (result == FF_SUCCESS)
	{
		wchar_t *buf;
		int new_file_path_len;

		ff_free((void *) blob->file_path);
		new_file_path_len = wcslen(new_file_path);
		buf = (wchar_t *) ff_calloc(new_file_path_len + 1, sizeof(blob->file_path[0]));
		memcpy(buf, new_file_path, new_file_path_len * sizeof(new_file_path[0]));
		blob->file_path = buf;
	}
	else
	{
		ff_log_warning(L"cannot move the blob backing file from the [%ls] to the [%ls]", blob->file_path, new_file_path);
	}
	mrpc_blob_dec_ref(blob);

	return result;
}

enum ff_result mrpc_blob_get_hash(struct mrpc_blob *blob, uint32_t start_value, uint32_t *hash_value)
{
	struct ff_stream *stream;
	enum ff_result result = FF_FAILURE;

	ff_assert(blob->state == BLOB_COMPLETE);
	ff_assert(blob->ref_cnt > 0);

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	if (stream != NULL)
	{
		int len;

		len = mrpc_blob_get_len(blob);
		result = ff_stream_get_hash(stream, len, start_value, hash_value);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot calculate hash value for stream=%p, len=%d, start_value=%lu. See previous messages for more info", stream, len, start_value);
		}
		ff_stream_delete(stream);
	}
	else
	{
		ff_log_debug(L"cannot open blob=%p stream for reading. See previous messages for more info", blob);
	}
	return result;
}

enum ff_result mrpc_blob_serialize(struct mrpc_blob *blob, struct ff_stream *stream)
{
	int len;
	struct ff_stream *blob_stream;
	enum ff_result result = FF_FAILURE;

	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	if (blob_stream == NULL)
	{
		ff_log_debug(L"cannot open blob stream for reading for the blob=%p. See previous messages for more info", blob);
		goto end;
	}

	len = mrpc_blob_get_len(blob);
	ff_assert(len >= 0);
	result = mrpc_uint32_serialize((uint32_t) len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"error occured while serializing blob length=%d into the stream=%p. See previous messages for more info", len, stream);
		ff_stream_delete(blob_stream);
		goto end;
	}

	result = ff_stream_copy(blob_stream, stream, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"error occured while copying blob_stream=%p contents to the stream=%p, len=%d. See previous messages for more info", blob_stream, stream, len);
	}
	ff_stream_delete(blob_stream);

end:
	return result;
}

enum ff_result mrpc_blob_unserialize(struct mrpc_blob **blob, struct ff_stream *stream)
{
	int len;
	struct mrpc_blob *new_blob;
	struct ff_stream *blob_stream;
	enum ff_result result;

	result = mrpc_uint32_unserialize((uint32_t *) &len, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"error occured while unserializing blob length form the stream=%p. See previous messages for more info", stream);
		goto end;
	}
	if (len < 0)
	{
		ff_log_debug(L"unexpected blob length=%d value read from the stream=%p. It must be posisitive", len, stream);
		goto end;
	}

	new_blob = mrpc_blob_create(len);
	blob_stream = mrpc_blob_open_stream(new_blob, MRPC_BLOB_WRITE);
	if (blob_stream == NULL)
	{
		ff_log_debug(L"cannot open blob stream for writing to the new_blob=%p, len=%d. See previous messages for more info", new_blob, len);
		mrpc_blob_dec_ref(new_blob);
		result = FF_FAILURE;
		goto end;
	}
	result = ff_stream_copy(stream, blob_stream, len);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot copy stream=%p to the blob_stream=%p, len=%d. See previous messages for more info", stream, blob_stream, len);
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
