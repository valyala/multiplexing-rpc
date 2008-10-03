#include "private/mrpc_common.h"

#include "private/mrpc_blob.h"
#include "ff/ff_stream.h"
#include "ff/ff_string.h"
#include "ff/ff_file.h"
#include "ff/ff_log.h"
#include "ff/arch/ff_arch_misc.h"

enum blob_state
{
	BLOB_EMPTY,
	BLOB_INCOMPLETE,
	BLOB_COMPLETE
};

struct mrpc_blob
{
	struct ff_string *file_path;
	int ref_cnt;
	int size;
	enum blob_state state;
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

static int read_from_blob_stream(struct ff_stream *stream, void *buf, int len)
{
	struct blob_stream_data *data;
	int bytes_left;
	int is_success = 0;

	ff_assert(len >= 0);

	data = (struct blob_stream_data *) ff_stream_get_ctx(stream);

	ff_assert(data->mode == MRPC_BLOB_READ);
	ff_assert(data->blob->state == BLOB_COMPLETE);
	ff_assert(data->curr_pos >= 0);
	ff_assert(data->curr_pos <= data->blob->size);
	bytes_left = data->blob->size - data->curr_pos;
	if (len <= bytes_left)
	{
		is_success = ff_file_read(data->file, buf, len);
		data->curr_pos += len;
	}
	return is_success;
}

static int write_to_blob_stream(struct ff_stream *stream, const void *buf, int len)
{
	struct blob_stream_data *data;
	int is_success = 0;

	ff_assert(len >= 0);

	data = (struct blob_stream_data *) ff_stream_get_ctx(stream);

	ff_assert(data->mode = MRPC_BLOB_WRITE);
	ff_assert(data->blob->state == BLOB_INCOMPLETE);
	ff_assert(data->curr_pos >= 0);
	ff_assert(data->curr_pos <= data->blob->size);
	bytes_left = data->blob->size - data->curr_pos;
	if (len <= bytes_left)
	{
		is_success = ff_file_write(data->file, buf, len);
		data->curr_pos += len;
	}
	if (data->curr_pos == data->blob->size)
	{
		data->blob->state = BLOB_COMPLETE;
	}
	return is_success;
}

static int flush_blob_stream(struct ff_stream *stream)
{
	struct blob_stream_data *data;
	int is_success;

	data = (Struct blob_stream_data *) ff_stream_get_cts(stream);

	ff_assert(data->mode == MRPC_BLOB_WRITE);
	ff_assert(data->blob->state == BLOB_INCOMPLETE);
	is_success = ff_file_flush(data->file);
	return is_success;
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

static struct ff_string *create_temporary_file_path()
{
	struct ff_string *file_path;

	file_path = ff_arch_misc_get_tmp_dir();
	ff_string_append_cstr(file_path, L"random_str");

	return file_path;
}

static struct mrpc_blob *create_blob(int size)
{
	struct mrpc_blob *blob;

	ff_assert(size >= 0);

	blob = (struct mrpc_blob *) ff_malloc(sizeof(*blob));
	blob->file_path = create_temporary_file_path();
	blob->ref_cnt = 1;
	blob->size = size;
	blob->state = BLOB_EMPTY;
	return blob;
}

static void delete_blob(struct mrpc_blob *blob)
{
	ff_assert(blob->ref_cnt == 0);

	if (blob->state != BLOB_EMPTY)
	{
		const wchar_t *file_path_cstr;
		int is_success;

		file_path_cstr = ff_string_get_cstr(file_path);
		is_success = ff_file_erase(file_path_cstr);
		if (!is_success)
		{
			ff_log_warning(L"cannot delete the blob backing file [%ls]", file_path_cstr);
		}
	}

	ff_string_dec_ref(blob->file_path);
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
	ff_assert(blob->ref_cnt > 0);
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
	ff_assert(blob->size >= 0);

	return blob->size;
}

struct ff_stream *mrpc_blob_open_stream(struct mrpc_blob *blob, enum mrpc_blob_open_stream_mode mode)
{
	struct ff_stream *stream = NULL;
	struct blob_stream_data *data;
	struct ff_file *file;
	const wchar_t *file_path_cstr;

	file_path_cstr = ff_string_get_cstr(blob->file_path);
	if (mode == MRPC_BLOB_READ)
	{
		ff_assert(blob->state == BLOB_COMPLETE);
		file = ff_file_open(file_path_cstr, FF_FILE_READ);
		if (file == NULL)
		{
			ff_log_warning(L"cannot open the blob backing file [%ls] for reading", file_path_cstr);
			goto end;
		}
	}
	else
	{
		ff_assert(mode == MRPC_BLOB_WRITE);
		ff_assert(blob->state == BLOB_EMPTY);
		file = ff_file_open(file_path_cstr, FF_FILE_WRITE);
		if (file == NULL)
		{
			ff_log_warning(L"cannot open the blob backing file [%ls] for writing", file_path_cstr);
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

int mrpc_blob_move(struct mrpc_blob *blob, struct ff_string *new_file_path)
{
	const wchar_t *src_path;
	const wchar_t *dst_path;
	int is_success;

	ff_assert(blob->state == BLOB_COMPLETE);
	ff_assert(blob->ref_cnt == 1);

	src_path = ff_string_get_cstr(blob->file_path);
	dst_path = ff_string_get_cstr(new_file_path);
	is_success = ff_file_move(src_path, dst_path);
	if (is_success)
	{
		ff_string_dec_ref(blob->file_path);
		blob->file_path = dst_filename;
		ff_string_inc_ref(dst_filename);
	}
	else
	{
		ff_log_warning(L"cannot move the blob backing file from the [%ls] to the [%ls]", src_path, dst_path);
	}

	return is_success;
}
