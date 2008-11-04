#include "mrpc/mrpc_char_array.h"
#include "mrpc/mrpc_wchar_array.h"
#include "mrpc/mrpc_blob.h"

#include "ff/ff_core.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"

#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
	#undef NDEBUG
	#include <assert.h>
	#define NDEBUG
#else
	#include <assert.h>
#endif

#define LOG_FILENAME L"mrpc_tests_log.txt"

#define ASSERT(expr, msg) assert((expr) && (msg))

#pragma region mrpc_char_array tests

static void test_char_array_create_delete()
{
	struct mrpc_char_array *char_array;
	char *s1;
	const char *s2;
	int len;

	s1 = (char *) ff_calloc(4, sizeof(s1[0]));
	memcpy(s1, "foo", 3 * sizeof(s1[0]));
	char_array = mrpc_char_array_create(s1, 3);
	ASSERT(char_array != NULL, "mrpc_char_array_create() cannot return NULL");
	len = mrpc_char_array_get_len(char_array);
	ASSERT(len == 3, "unexpected length of the char array received");
	s2 = mrpc_char_array_get_value(char_array);
	ASSERT(s1 == s2, "unexpected value received from the char_array");
	mrpc_char_array_dec_ref(char_array);
	/* mrpc_charr_array_dec_ref() must delete the memory allocated for the s1 */
}

static void test_char_array_basic()
{
	struct mrpc_char_array *char_array;
	char *s;
	uint32_t hash_value;
	int i;

	s = (char *) ff_calloc(6, sizeof(s[0]));
	memcpy(s, "abcde", 5 * sizeof(s[0]));
	char_array = mrpc_char_array_create(s, 5);

	for (i = 0; i < 10; i++)
	{
		mrpc_char_array_inc_ref(char_array);
	}
	mrpc_char_array_dec_ref(char_array);
	mrpc_char_array_inc_ref(char_array);
	for (i = 0; i < 10; i++)
	{
		mrpc_char_array_dec_ref(char_array);
	}

	hash_value = mrpc_char_array_get_hash(char_array, 123);
	ASSERT(hash_value == 3133663974ul, "unexpected hash value");

	mrpc_char_array_dec_ref(char_array);
}

static void test_char_array_all()
{
	ff_core_initialize(LOG_FILENAME);

	test_char_array_create_delete();
	test_char_array_basic();

	ff_core_shutdown();
}

/* end of mrpc_char_array tests */
#pragma endregion


#pragma region mrpc_wchar_array tests

static void test_wchar_array_create_delete()
{
	struct mrpc_wchar_array *wchar_array;
	wchar_t *s1;
	const wchar_t *s2;
	int len;

	s1 = (wchar_t *) ff_calloc(4, sizeof(s1[0]));
	memcpy(s1, L"foo", 3 * sizeof(s1[0]));
	wchar_array = mrpc_wchar_array_create(s1, 3);
	ASSERT(wchar_array != NULL, "mrpc_wchar_array_create() cannot return NULL");
	len = mrpc_wchar_array_get_len(wchar_array);
	ASSERT(len == 3, "unexpected length of the wchar array received");
	s2 = mrpc_wchar_array_get_value(wchar_array);
	ASSERT(s1 == s2, "unexpected value received from the wchar_array");
	mrpc_wchar_array_dec_ref(wchar_array);
	/* mrpc_wcharr_array_dec_ref() must delete the memory allocated for the s1 */
}

static void test_wchar_array_basic()
{
	struct mrpc_wchar_array *wchar_array;
	wchar_t *s;
	uint32_t hash_value;
	int i;

	s = (wchar_t *) ff_calloc(6, sizeof(s[0]));
	memcpy(s, L"abcde", 5 * sizeof(s[0]));
	wchar_array = mrpc_wchar_array_create(s, 5);

	for (i = 0; i < 10; i++)
	{
		mrpc_wchar_array_inc_ref(wchar_array);
	}
	mrpc_wchar_array_dec_ref(wchar_array);
	mrpc_wchar_array_inc_ref(wchar_array);
	for (i = 0; i < 10; i++)
	{
		mrpc_wchar_array_dec_ref(wchar_array);
	}

	hash_value = mrpc_wchar_array_get_hash(wchar_array, 123);
	ASSERT(hash_value == 2051898534ul, "unexpected hash value");

	mrpc_wchar_array_dec_ref(wchar_array);
}

static void test_wchar_array_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_wchar_array_create_delete();
	test_wchar_array_basic();
	ff_core_shutdown();
}

/* end of mrpc_char_array tests */
#pragma endregion


#pragma region mrpc_blob tests

static void test_blob_create_delete()
{
	struct mrpc_blob *blob;
	int size;

	blob = mrpc_blob_create(10);
	ASSERT(blob != NULL, "blob cannot be null");
	size = mrpc_blob_get_size(blob);
	ASSERT(size == 10, "unexpected size for the blob");
	mrpc_blob_dec_ref(blob);
}

static void test_blob_basic()
{
	struct mrpc_blob *blob;
	struct ff_stream *stream;
	char buf[5];
	uint32_t hash_value;
	int i;
	int is_equal;
	enum ff_result result;

	blob = mrpc_blob_create(10);
	for (i = 0; i < 10; i++)
	{
		mrpc_blob_inc_ref(blob);
	}
	mrpc_blob_dec_ref(blob);
	mrpc_blob_inc_ref(blob);
	for (i = 0; i < 10; i++)
	{
		mrpc_blob_dec_ref(blob);
	}

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
	ASSERT(stream != NULL, "cannot open blob stream for writing");
	result = ff_stream_write(stream, "1234", 4);
	ASSERT(result == FF_SUCCESS, "cannot write data to blob stream");
	result = ff_stream_write(stream, "5678", 4);
	ASSERT(result == FF_SUCCESS, "cannot write data to blob stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the blob stream");
	result = ff_stream_write(stream, "90", 2);
	ASSERT(result == FF_SUCCESS, "cannot write data to blob stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the blob stream");
	result = ff_stream_write(stream, "junk", 4);
	ASSERT(result != FF_SUCCESS, "unexpected result on attempt of writing more data than the blob capacity");
	ff_stream_delete(stream);

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	ASSERT(stream != NULL, "cannot open blob stream for reading");
	result = ff_stream_read(stream, buf, 5);
	ASSERT(result == FF_SUCCESS, "cannot read data from the stream");
	is_equal = (memcmp(buf, "12345", 5) == 0);
	ASSERT(is_equal, "unexpected data read from the stream");
	result = ff_stream_read(stream, buf, 5);
	ASSERT(result == FF_SUCCESS, "cannot read data from the stream");
	is_equal = (memcmp(buf, "67890", 5) == 0);
	ASSERT(is_equal, "unexpected data read from the stream");
	result = ff_stream_read(stream, buf, 3);
	ASSERT(result != FF_SUCCESS, "unexpected result on attempt of reading more data than the blob capacity");
	ff_stream_delete(stream);

	result = mrpc_blob_move(blob, L"test_blob_file.txt");
	ASSERT(result == FF_SUCCESS, "cannot move the blob file");

	hash_value = mrpc_blob_get_hash(blob, 432);
	ASSERT(hash_value == 3858857425, "wrong hash value calculated for the blob");

	mrpc_blob_dec_ref(blob);
}

struct blob_multiple_read_data
{
	struct ff_event *event;
	struct mrpc_blob *blob;
	int workers_cnt;
};

static void blob_multiple_read_fiberpool_func(void *ctx)
{
	struct blob_multiple_read_data *data;
	struct ff_stream *stream;
	char buf[10];
	int is_equal;
	enum ff_result result;

	data = (struct blob_multiple_read_data *) ctx;

	stream = mrpc_blob_open_stream(data->blob, MRPC_BLOB_READ);
	ASSERT(stream != NULL, "cannot open blob stream for reading");
	result = ff_stream_read(stream, buf, 10);
	ASSERT(result == FF_SUCCESS, "cannot read data from the blob stream");
	is_equal = (memcmp(buf, "1234567890", 10) == 0);
	ASSERT(is_equal, "unexpected data read from the blob stream");
	ff_stream_delete(stream);

	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void test_blob_multiple_read()
{
	struct mrpc_blob *blob;
	struct ff_stream *stream;
	struct blob_multiple_read_data data;
	int i;
	enum ff_result result;

	blob = mrpc_blob_create(10);

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
	ASSERT(stream != NULL, "cannot open blob stream for writing");
	result = ff_stream_write(stream, "1234567890", 10);
	ASSERT(result == FF_SUCCESS, "cannot write data to the blob stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the blob stream");
	ff_stream_delete(stream);

	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.blob = blob;
	data.workers_cnt = 10;
	for (i = 0; i < 10; i++)
	{
		ff_core_fiberpool_execute_async(blob_multiple_read_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);

	mrpc_blob_dec_ref(blob);
}

static void test_blob_ref_cnt()
{
	struct mrpc_blob *blob;
	struct ff_stream *stream;
	enum ff_result result;

	blob = mrpc_blob_create(10);

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
	ASSERT(stream != NULL, "cannot open blob stream for writing");
	result = ff_stream_write(stream, "1234567890", 10);
	ASSERT(result == FF_SUCCESS, "cannot write data to the blob stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the blob stream");
	ff_stream_delete(stream);

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	mrpc_blob_dec_ref(blob);
	mrpc_blob_inc_ref(blob);
	ff_stream_delete(stream);

	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	mrpc_blob_dec_ref(blob);
	ff_stream_delete(stream);
}

static void test_blob_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_blob_create_delete();
	test_blob_basic();
	test_blob_multiple_read();
	test_blob_ref_cnt();
	ff_core_shutdown();
}

/* end of mrpc_blob tests */
#pragma endregion

static void test_all()
{
	test_char_array_all();
	test_wchar_array_all();
	test_blob_all();
}

int main(int argc, char* argv[])
{
	test_all();
	printf("ALL TESTS PASSED\n");

	return 0;
}
