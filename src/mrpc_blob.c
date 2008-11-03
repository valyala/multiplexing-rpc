#include "private/mrpc_common.h"

#include "private/mrpc_blob.h"
#include "ff/ff_stream.h"
#include "ff/ff_file.h"
#include "ff/ff_log.h"
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
#define BLOB_FILENAME_PREFIX L"blob."
#define BLOB_FILENAME_PREFIX_LEN (sizeof(BLOB_FILENAME_PREFIX) / sizeof(BLOB_FILENAME_PREFIX[0]))

enum blob_state
{
	BLOB_EMPTY,
	BLOB_INCOMPLETE,
	BLOB_COMPLETE
};

struct mrpc_blob
{
	const wchar_t *file_path;
	int size;
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
	ff_assert(data->curr_pos <= data->blob->size);
	bytes_left = data->blob->size - data->curr_pos;
	if (len <= bytes_left)
	{
		result = ff_file_read(data->file, buf, len);
		if (result == FF_SUCCESS)
		{
			data->curr_pos += len;
		}
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
	ff_assert(data->blob->state == BLOB_INCOMPLETE);
	ff_assert(data->curr_pos >= 0);
	ff_assert(data->curr_pos <= data->blob->size);
	bytes_left = data->blob->size - data->curr_pos;
	if (len <= bytes_left)
	{
		result = ff_file_write(data->file, buf, len);
		if (result == FF_SUCCESS)
		{
			data->curr_pos += len;
			if (data->curr_pos == data->blob->size)
			{
				data->blob->state = BLOB_COMPLETE;
			}
		}
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

static struct mrpc_blob *create_blob(int size)
{
	struct mrpc_blob *blob;

	ff_assert(size >= 0);

	blob = (struct mrpc_blob *) ff_malloc(sizeof(*blob));
	blob->file_path = create_temporary_file_path();
	blob->size = size;
	blob->state = BLOB_EMPTY;
	blob->ref_cnt = 1;
	return blob;
}

static void delete_blob(struct mrpc_blob *blob)
{
	enum ff_result result;

	ff_assert(blob->ref_cnt == 0);
	ff_assert(blob->state != BLOB_EMPTY);

	result = ff_file_erase(blob->file_path);
	if (result != FF_SUCCESS)
	{
		ff_log_warning(L"cannot delete the blob backing file [%ls]", blob->file_path);
	}

	ff_free((void *) blob->file_path);
	ff_free(blob);
}

struct mrpc_blob *mrpc_blob_create(int size)
{
	struct mrpc_blob *blob;

	ff_assert(size >= 0);

	blob = create_blob(size);
	return blob;
}

void mrpc_blob_inc_ref(struct mrpc_blob *blob)
{
	blob->ref_cnt++;
	ff_assert(blob->ref_cnt > 0);
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

int mrpc_blob_get_size(struct mrpc_blob *blob)
{
	ff_assert(blob->ref_cnt > 0);
	ff_assert(blob->size >= 0);

	return blob->size;
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
		file = ff_file_open(blob->file_path, FF_FILE_READ);
		if (file == NULL)
		{
			ff_log_warning(L"cannot open the blob backing file [%ls] for reading", blob->file_path);
			goto end;
		}
	}
	else
	{
		ff_assert(mode == MRPC_BLOB_WRITE);
		ff_assert(blob->ref_cnt == 1);
		ff_assert(blob->state == BLOB_EMPTY);
		file = ff_file_open(blob->file_path, FF_FILE_WRITE);
		if (file == NULL)
		{
			ff_log_warning(L"cannot open the blob backing file [%ls] for writing", blob->file_path);
			goto end;
		}
		blob->state = BLOB_INCOMPLETE;
	}

	data = (struct blob_stream_data *) ff_malloc(sizeof(*data));
	data->blob = blob;
	mrpc_blob_inc_ref(data->blob);
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

	return result;
}

uint32_t mrpc_blob_get_hash(struct mrpc_blob *blob, uint32_t start_value)
{
	struct ff_stream *stream;
	uint32_t hash_value;
	enum ff_result result = FF_FAILURE;

	ff_assert(blob->state == BLOB_COMPLETE);

	hash_value = start_value;
	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	if (stream != NULL)
	{
		result = ff_stream_get_hash(stream, blob->size, start_value, &hash_value);
		ff_stream_delete(stream);
	}
	if (result != FF_SUCCESS)
	{
		ff_log_warning(L"cannot calculate hash value for the blob. See previous message for details");
	}
	return hash_value;
}
