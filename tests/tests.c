#include "mrpc/mrpc_char_array.h"
#include "mrpc/mrpc_wchar_array.h"
#include "mrpc/mrpc_blob.h"
#include "mrpc/mrpc_char_array_param.h"
#include "mrpc/mrpc_wchar_array_param.h"
#include "mrpc/mrpc_param.h"

#include "ff/ff_core.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"
#include "ff/ff_stream_connector_tcp.h"
#include "ff/ff_endpoint_tcp.h"
#include "ff/arch/ff_arch_net_addr.h"

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


#pragma region mrpc_char_array_param tests

static void test_char_array_param_create_delete()
{
	struct mrpc_param *param;

	param = mrpc_char_array_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);
}

static void char_array_param_basic_fiberpool_func(void *ctx)
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_endpoint *endpoint;
	struct ff_stream *stream;
	struct mrpc_param *param;
	enum ff_result result;

	event = (struct ff_event *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8490);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	endpoint = ff_endpoint_tcp_create(addr);
	ff_endpoint_initialize(endpoint);
	stream = ff_endpoint_accept(endpoint);
	ASSERT(stream != NULL, "cannot accept local connection");

	param = mrpc_char_array_param_create();
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read char array from the stream");
	result = mrpc_param_write_to_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write char array to the stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	mrpc_param_delete(param);
	ff_stream_delete(stream);
	ff_endpoint_shutdown(endpoint);
	ff_endpoint_delete(endpoint);

	ff_event_set(event);
}

static void test_char_array_param_basic()
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *connector;
	struct ff_stream *stream;
	struct mrpc_char_array *char_array1;
	struct mrpc_char_array *char_array2;
	struct mrpc_char_array *char_array3;
	struct mrpc_param *param;
	char *s1;
	const char *s2;
	int len;
	int is_equal;
	enum ff_result result;

	event = ff_event_create(FF_EVENT_MANUAL);
	ff_core_fiberpool_execute_async(char_array_param_basic_fiberpool_func, event);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8490);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	connector = ff_stream_connector_tcp_create(addr);
	stream = ff_stream_connector_connect(connector);
	ASSERT(stream != NULL, "cannot establish connection to localhost");

	s1 = (char *) ff_calloc(8, sizeof(s1[0]));
	memcpy(s1, "foo bar", 7 * sizeof(s1[0]));
	char_array1 = mrpc_char_array_create(s1, 7);

	param = mrpc_char_array_param_create();
	mrpc_param_set_value(param, char_array1);
	mrpc_param_get_value(param, (void **) &char_array2);
	ASSERT(char_array1 == char_array2, "wrong value obtained from the parameter");
	result = mrpc_param_write_to_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write char array to the stream");
	result = ff_stream_flush(stream);
	mrpc_param_delete(param);

	param = mrpc_char_array_param_create();
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read char array from the stream");

	mrpc_param_get_value(param, (void **) &char_array3);
	len = mrpc_char_array_get_len(char_array3);
	ASSERT(len == 7, "unexpected length of the char array");
	s2 = mrpc_char_array_get_value(char_array3);
	is_equal = (memcmp(s2, "foo bar", 7 * sizeof(s2[0])) == 0);
	ASSERT(is_equal, "unexpected value of the char array");
	mrpc_param_delete(param);

	ff_stream_delete(stream);
	ff_stream_connector_delete(connector);

	ff_event_wait(event);
	ff_event_delete(event);
}

static void test_char_array_param_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_char_array_param_create_delete();
	test_char_array_param_basic();
	ff_core_shutdown();
}

/* end of mrpc_char_array_param tests */
#pragma endregion


#pragma region mrpc_wchar_array_param tests

static void test_wchar_array_param_create_delete()
{
	struct mrpc_param *param;

	param = mrpc_wchar_array_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);
}

static void wchar_array_param_basic_fiberpool_func(void *ctx)
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_endpoint *endpoint;
	struct ff_stream *stream;
	struct mrpc_param *param;
	enum ff_result result;

	event = (struct ff_event *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8490);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	endpoint = ff_endpoint_tcp_create(addr);
	ff_endpoint_initialize(endpoint);
	stream = ff_endpoint_accept(endpoint);
	ASSERT(stream != NULL, "cannot accept local connection");

	param = mrpc_wchar_array_param_create();
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read wchar array from the stream");
	result = mrpc_param_write_to_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write wchar array to the stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	mrpc_param_delete(param);
	ff_stream_delete(stream);
	ff_endpoint_shutdown(endpoint);
	ff_endpoint_delete(endpoint);

	ff_event_set(event);
}

static void test_wchar_array_param_basic()
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *connector;
	struct ff_stream *stream;
	struct mrpc_wchar_array *wchar_array1;
	struct mrpc_wchar_array *wchar_array2;
	struct mrpc_wchar_array *wchar_array3;
	struct mrpc_param *param;
	wchar_t *s1;
	const wchar_t *s2;
	int len;
	int is_equal;
	enum ff_result result;

	event = ff_event_create(FF_EVENT_MANUAL);
	ff_core_fiberpool_execute_async(char_array_param_basic_fiberpool_func, event);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8490);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	connector = ff_stream_connector_tcp_create(addr);
	stream = ff_stream_connector_connect(connector);
	ASSERT(stream != NULL, "cannot establish connection to localhost");

	s1 = (wchar_t *) ff_calloc(8, sizeof(s1[0]));
	memcpy(s1, L"foo bar", 7 * sizeof(s1[0]));
	wchar_array1 = mrpc_wchar_array_create(s1, 7);

	param = mrpc_wchar_array_param_create();
	mrpc_param_set_value(param, wchar_array1);
	mrpc_param_get_value(param, (void **) &wchar_array2);
	ASSERT(wchar_array1 == wchar_array2, "wrong value obtained from the parameter");
	result = mrpc_param_write_to_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write wchar array to the stream");
	result = ff_stream_flush(stream);
	mrpc_param_delete(param);

	param = mrpc_wchar_array_param_create();
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read wchar array from the stream");

	mrpc_param_get_value(param, (void **) &wchar_array3);
	len = mrpc_wchar_array_get_len(wchar_array3);
	ASSERT(len == 7, "unexpected length of the wchar array");
	s2 = mrpc_wchar_array_get_value(wchar_array3);
	is_equal = (memcmp(s2, L"foo bar", 7 * sizeof(s2[0])) == 0);
	ASSERT(is_equal, "unexpected value of the wchar array");
	mrpc_param_delete(param);

	ff_stream_delete(stream);
	ff_stream_connector_delete(connector);

	ff_event_wait(event);
	ff_event_delete(event);
}

static void test_wchar_array_param_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_wchar_array_param_create_delete();
	test_wchar_array_param_basic();
	ff_core_shutdown();
}

/* end of mrpc_char_array_param tests */
#pragma endregion


static void test_all()
{
	test_char_array_all();
	test_wchar_array_all();
	test_blob_all();
	test_char_array_param_all();
	test_wchar_array_param_all();
}

int main(int argc, char* argv[])
{
	test_all();
	printf("ALL TESTS PASSED\n");

	return 0;
}
