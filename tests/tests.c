#include "mrpc/mrpc_char_array.h"
#include "mrpc/mrpc_wchar_array.h"
#include "mrpc/mrpc_blob.h"
#include "mrpc/mrpc_int_param.h"
#include "mrpc/mrpc_char_array_param.h"
#include "mrpc/mrpc_wchar_array_param.h"
#include "mrpc/mrpc_blob_param.h"
#include "mrpc/mrpc_param.h"
#include "mrpc/mrpc_method_factory.h"
#include "mrpc/mrpc_method.h"
#include "mrpc/mrpc_interface.h"
#include "mrpc/mrpc_data.h"
#include "mrpc/mrpc_client.h"
#include "mrpc/mrpc_server.h"

#include "ff/ff_core.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"
#include "ff/ff_stream_connector_tcp.h"
#include "ff/ff_stream_acceptor_tcp.h"
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
	ASSERT(hash_value == 3858857425ul, "wrong hash value calculated for the blob");

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


#pragma region mrpc_int_param tests

static void test_int_param_create_delete()
{
	struct mrpc_param *param;

	param = mrpc_uint32_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);

	param = mrpc_int32_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);

	param = mrpc_uint64_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);

	param = mrpc_int64_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);
}

static void int_param_basic_fiberpool_func(void *ctx)
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_param *u32_param;
	struct mrpc_param *s32_param;
	struct mrpc_param *u64_param;
	struct mrpc_param *s64_param;
	enum ff_result result;

	event = (struct ff_event *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8489);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	ff_event_set(event);
	stream = ff_stream_acceptor_accept(stream_acceptor);
	ASSERT(stream != NULL, "cannot accept local connection");

	u32_param = mrpc_uint32_param_create();
	s32_param = mrpc_int32_param_create();
	u64_param = mrpc_uint64_param_create();
	s64_param = mrpc_int64_param_create();

	result = mrpc_param_read_from_stream(u32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read u32 from the stream");
	result = mrpc_param_read_from_stream(s32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read s32 from the stream");
	result = mrpc_param_read_from_stream(u64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read u64 from the stream");
	result = mrpc_param_read_from_stream(s64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read s64 from the stream");

	result = mrpc_param_write_to_stream(u32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write u32 to the stream");
	result = mrpc_param_write_to_stream(s32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write s32 to the stream");
	result = mrpc_param_write_to_stream(u64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write u64 to the stream");
	result = mrpc_param_write_to_stream(s64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write s64 to the stream");

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	mrpc_param_delete(u32_param);
	mrpc_param_delete(s32_param);
	mrpc_param_delete(u64_param);
	mrpc_param_delete(s64_param);

	ff_stream_delete(stream);
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

	ff_event_set(event);
}

static void test_int_param_basic()
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *connector;
	struct ff_stream *stream;
	struct mrpc_param *u32_param;
	struct mrpc_param *s32_param;
	struct mrpc_param *u64_param;
	struct mrpc_param *s64_param;
	uint32_t u32_value, *u32_ptr;
	int32_t s32_value, *s32_ptr;
	uint64_t u64_value, *u64_ptr;
	int64_t s64_value, *s64_ptr;
	uint32_t hash_value;
	enum ff_result result;

	event = ff_event_create(FF_EVENT_AUTO);
	ff_core_fiberpool_execute_async(int_param_basic_fiberpool_func, event);
	ff_event_wait(event);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8489);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(connector);
	stream = ff_stream_connector_connect(connector);
	ASSERT(stream != NULL, "cannot establish connection to localhost");

	u32_param = mrpc_uint32_param_create();
	s32_param = mrpc_int32_param_create();
	u64_param = mrpc_uint64_param_create();
	s64_param = mrpc_int64_param_create();

	u32_value = 12345678ul;
	s32_value = -2345870l;
	u64_value = 12345678902343ull;
	s64_value = -34823472894342ll;

	mrpc_param_set_value(u32_param, &u32_value);
	mrpc_param_get_value(u32_param, (void **) &u32_ptr);
	ASSERT(u32_value == *u32_ptr, "wrong value obtained from the parameter");
	mrpc_param_set_value(s32_param, &s32_value);
	mrpc_param_get_value(s32_param, (void **) &s32_ptr);
	ASSERT(s32_value == *s32_ptr, "wrong value obtained from the parameter");
	mrpc_param_set_value(u64_param, &u64_value);
	mrpc_param_get_value(u64_param, (void **) &u64_ptr);
	ASSERT(u64_value == *u64_ptr, "wrong value obtained from the parameter");
	mrpc_param_set_value(s64_param, &s64_value);
	mrpc_param_get_value(s64_param, (void **) &s64_ptr);
	ASSERT(s64_value == *s64_ptr, "wrong value obtained from the parameter");

	result = mrpc_param_write_to_stream(u32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write u32 to the stream");
	result = mrpc_param_write_to_stream(s32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write s32 to the stream");
	result = mrpc_param_write_to_stream(u64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write u64 to the stream");
	result = mrpc_param_write_to_stream(s64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write s64 to the stream");

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	mrpc_param_delete(u32_param);
	mrpc_param_delete(s32_param);
	mrpc_param_delete(u64_param);
	mrpc_param_delete(s64_param);

	u32_param = mrpc_uint32_param_create();
	s32_param = mrpc_int32_param_create();
	u64_param = mrpc_uint64_param_create();
	s64_param = mrpc_int64_param_create();

	result = mrpc_param_read_from_stream(u32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read u32 from the stream");
	result = mrpc_param_read_from_stream(s32_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read s32 from the stream");
	result = mrpc_param_read_from_stream(u64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read u64 from the stream");
	result = mrpc_param_read_from_stream(s64_param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read s64 from the stream");

	mrpc_param_get_value(u32_param, (void **) &u32_ptr);
	ASSERT(u32_value == *u32_ptr, "unexpected value of the u32");
	mrpc_param_get_value(s32_param, (void **) &s32_ptr);
	ASSERT(s32_value == *s32_ptr, "unexpected value of the s32");
	mrpc_param_get_value(u64_param, (void **) &u64_ptr);
	ASSERT(u64_value == *u64_ptr, "unexpected value of the u64");
	mrpc_param_get_value(s64_param, (void **) &s64_ptr);
	ASSERT(s64_value == *s64_ptr, "unexpected value of the s64");

	hash_value = mrpc_param_get_hash(u32_param, 12345);
	ASSERT(hash_value == 3806499569ul, "unexpected hash value");
	hash_value = mrpc_param_get_hash(s32_param, 12345);
	ASSERT(hash_value == 138116614ul, "unexpected hash value");
	hash_value = mrpc_param_get_hash(u64_param, 12345);
	ASSERT(hash_value == 3433180561ul, "unexpected hash value");
	hash_value = mrpc_param_get_hash(s64_param, 12345);
	ASSERT(hash_value == 647666836ul, "unexpected hash value");

	mrpc_param_delete(u32_param);
	mrpc_param_delete(s32_param);
	mrpc_param_delete(u64_param);
	mrpc_param_delete(s64_param);

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(connector);
	ff_stream_connector_delete(connector);

	ff_event_wait(event);
	ff_event_delete(event);
}

static void test_int_param_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_int_param_create_delete();
	test_int_param_basic();
	ff_core_shutdown();
}

/* end of mrpc_int_param tests */
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
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_param *param;
	enum ff_result result;

	event = (struct ff_event *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8490);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	ff_event_set(event);
	stream = ff_stream_acceptor_accept(stream_acceptor);
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
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

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
	uint32_t hash_value;
	int len;
	int is_equal;
	enum ff_result result;

	event = ff_event_create(FF_EVENT_AUTO);
	ff_core_fiberpool_execute_async(char_array_param_basic_fiberpool_func, event);
	ff_event_wait(event);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8490);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(connector);
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
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");
	mrpc_param_delete(param);

	param = mrpc_char_array_param_create();
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read char array from the stream");

	mrpc_param_get_value(param, (void **) &char_array3);
	len = mrpc_char_array_get_len(char_array3);
	ASSERT(len == 7, "unexpected length of the char array");
	s2 = mrpc_char_array_get_value(char_array3);
	is_equal = (memcmp(s2, "foo bar", 7 * sizeof(s2[0])) == 0);
	ASSERT(is_equal, "unexpected value of the char array");
	hash_value = mrpc_param_get_hash(param, 12345);
	ASSERT(hash_value == 1444866618ul, "unexpected hash value");
	mrpc_param_delete(param);

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(connector);
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
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_param *param;
	enum ff_result result;

	event = (struct ff_event *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8491);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	ff_event_set(event);
	stream = ff_stream_acceptor_accept(stream_acceptor);
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
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

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
	uint32_t hash_value;
	int len;
	int is_equal;
	enum ff_result result;

	event = ff_event_create(FF_EVENT_AUTO);
	ff_core_fiberpool_execute_async(wchar_array_param_basic_fiberpool_func, event);
	ff_event_wait(event);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8491);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(connector);
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
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");
	mrpc_param_delete(param);

	param = mrpc_wchar_array_param_create();
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read wchar array from the stream");

	mrpc_param_get_value(param, (void **) &wchar_array3);
	len = mrpc_wchar_array_get_len(wchar_array3);
	ASSERT(len == 7, "unexpected length of the wchar array");
	s2 = mrpc_wchar_array_get_value(wchar_array3);
	is_equal = (memcmp(s2, L"foo bar", 7 * sizeof(s2[0])) == 0);
	ASSERT(is_equal, "unexpected value of the wchar array");
	hash_value = mrpc_param_get_hash(param, 12345);
	ASSERT(hash_value == 3358053767ul, "unexpected hash value");
	mrpc_param_delete(param);

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(connector);
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

/* end of mrpc_wchar_array_param tests */
#pragma endregion


#pragma region mrpc_blob_param tests

static void test_blob_param_create_delete()
{
	struct mrpc_param *param;

	param = mrpc_blob_param_create();
	ASSERT(param != NULL, "param cannot be NULL");
	mrpc_param_delete(param);
}

static void blob_param_basic_fiberpool_func(void *ctx)
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_param *param;
	enum ff_result result;

	event = (struct ff_event *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8492);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	ff_event_set(event);
	stream = ff_stream_acceptor_accept(stream_acceptor);
	ASSERT(stream != NULL, "cannot accept local connection");

	param = mrpc_blob_param_create();
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read blob from the stream");
	result = mrpc_param_write_to_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write blob to the stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	mrpc_param_delete(param);
	ff_stream_delete(stream);
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

	ff_event_set(event);
}

static void test_blob_param_basic()
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *connector;
	struct ff_stream *stream;
	struct ff_stream *blob_stream;
	struct mrpc_blob *blob1;
	struct mrpc_blob *blob2;
	struct mrpc_blob *blob3;
	struct mrpc_param *param;
	uint32_t hash_value;
	char buf[10];
	int size;
	int is_equal;
	enum ff_result result;

	event = ff_event_create(FF_EVENT_AUTO);
	ff_core_fiberpool_execute_async(blob_param_basic_fiberpool_func, event);
	ff_event_wait(event);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8492);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(connector);
	stream = ff_stream_connector_connect(connector);
	ASSERT(stream != NULL, "cannot establish connection to localhost");

	blob1 = mrpc_blob_create(10);
	blob_stream = mrpc_blob_open_stream(blob1, MRPC_BLOB_WRITE);
	ASSERT(blob_stream != NULL, "cannot open blob stream for writing");
	result = ff_stream_write(blob_stream, "0123456789", 10);
	ASSERT(result == FF_SUCCESS, "cannot write data to blob stream");
	result = ff_stream_flush(blob_stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the blob stream");
	ff_stream_delete(blob_stream);

	param = mrpc_blob_param_create();
	mrpc_param_set_value(param, blob1);
	mrpc_param_get_value(param, (void **) &blob2);
	ASSERT(blob1 == blob2, "wrong value obtained from the parameter");
	result = mrpc_param_write_to_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot write blob to the stream");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");
	mrpc_param_delete(param);

	param = mrpc_blob_param_create();
	result = mrpc_param_read_from_stream(param, stream);
	ASSERT(result == FF_SUCCESS, "cannot read blob from the stream");

	mrpc_param_get_value(param, (void **) &blob3);
	size = mrpc_blob_get_size(blob3);
	ASSERT(size == 10, "unexpected size of the blob");
	blob_stream = mrpc_blob_open_stream(blob3, MRPC_BLOB_READ);
	ASSERT(blob_stream != NULL, "cannot open blob for reading");
	result = ff_stream_read(blob_stream, buf, 10);
	ASSERT(result == FF_SUCCESS, "cannot read data from the blob");
	is_equal = (memcmp(buf, "0123456789", 10) == 0);
	ASSERT(is_equal, "unexpected value of blob");
	ff_stream_delete(blob_stream);
	hash_value = mrpc_param_get_hash(param, 12345);
	ASSERT(hash_value == 318893864ul, "unexpected hash value");
	mrpc_param_delete(param);

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(connector);
	ff_stream_connector_delete(connector);

	ff_event_wait(event);
	ff_event_delete(event);
}

static void test_blob_param_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_blob_param_create_delete();
	test_blob_param_basic();
	ff_core_shutdown();
}

/* end of mrpc_blob_param tests */
#pragma endregion

#pragma region mrpc_method tests

static void test_method_client_create_delete()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_uint32_param_create,
		mrpc_int32_param_create,
		mrpc_uint64_param_create,
		mrpc_int64_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_blob_param_create,
		mrpc_wchar_array_param_create,
		mrpc_char_array_param_create,
		NULL,
	};
	static const int is_key[] =
	{
		1,
		0,
		0,
		1,
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "cannot create client method");
	mrpc_method_delete(method);
}

static void method_server_create_delete_callback(struct mrpc_data *data, void *service_ctx)
{
	ASSERT(0, "this callback shouldn't be called");
}

static void test_method_server_create_delete()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_blob_param_create,
		NULL,
	};

	method = mrpc_method_create_server_method(request_param_constructors, response_param_constructors, method_server_create_delete_callback);
	ASSERT(method != NULL, "cannot create server method");
	mrpc_method_delete(method);
}

static void test_method_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_method_client_create_delete();
	test_method_server_create_delete();
	ff_core_shutdown();
}

/* end of mrpc_method tests */
#pragma endregion

#pragma region mrpc_interface tests

static struct mrpc_method *interface_create_delete_method_constructor1()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_uint32_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		NULL,
	};
	static const int is_key[] =
	{
		0,
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static struct mrpc_method *interface_create_delete_method_constructor2()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_wchar_array_param_create,
		mrpc_uint32_param_create,
		mrpc_uint64_param_create,
		NULL,
	};
	static const int is_key[] =
	{
		0, /* fake value */
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static void test_interface_create_delete()
{
	struct mrpc_interface *interface;
	static const mrpc_method_constructor method_constructors[] =
	{
		interface_create_delete_method_constructor1,
		interface_create_delete_method_constructor2,
		NULL,
	};

	interface = mrpc_interface_create(method_constructors);
	ASSERT(interface != NULL, "mrpc_interface_create() must return valid result");
	mrpc_interface_delete(interface);
}

static void test_interface_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_interface_create_delete();
	ff_core_shutdown();
}

/* end of mrpc_interface tests */
#pragma endregion

#pragma region mrpc_data tests

static struct mrpc_method *data_method_constructor1_client()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_uint32_param_create,
		mrpc_uint64_param_create,
		mrpc_wchar_array_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_char_array_param_create,
		mrpc_blob_param_create,
		mrpc_uint64_param_create,
		NULL,
	};
	static const int is_key[] =
	{
		0,
		1,
		1
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static struct mrpc_method *data_method_constructor2_client()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_int32_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		NULL,
	};
	static const int is_key[] =
	{
		0,
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static const mrpc_method_constructor data_method_constructors_client[] =
{
	data_method_constructor1_client,
	data_method_constructor2_client,
	NULL,
};

static void data_method_callback1(struct mrpc_data *data, void *service_ctx)
{
	ASSERT(0, "this callback shouldn't be called");
}

static void data_method_callback2(struct mrpc_data *data, void *service_ctx)
{
	ASSERT(0, "this callback shouldn't be called");
}

static struct mrpc_method *data_method_constructor1_server()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_int64_param_create,
		mrpc_char_array_param_create,
		mrpc_int32_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_uint32_param_create,
		mrpc_uint64_param_create,
		mrpc_wchar_array_param_create,
		NULL,
	};

	method = mrpc_method_create_server_method(request_param_constructors, response_param_constructors, data_method_callback1);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static struct mrpc_method *data_method_constructor2_server()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_int32_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		NULL,
	};

	method = mrpc_method_create_server_method(request_param_constructors, response_param_constructors, data_method_callback2);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static const mrpc_method_constructor data_method_constructors_server[] =
{
	data_method_constructor1_server,
	data_method_constructor2_server,
	NULL,
};

static void test_data_create_delete()
{
	struct mrpc_interface *interface;
	struct mrpc_data *data;

	interface = mrpc_interface_create(data_method_constructors_client);
	ASSERT(interface != NULL, "interface cannot be NULL");

	data = mrpc_data_create(interface, 0);
	ASSERT(data != NULL, "data cannot be NULL");
	mrpc_data_delete(data);

	data = mrpc_data_create(interface, 1);
	ASSERT(data != NULL, "data cannot be NULL");
	mrpc_data_delete(data);

	mrpc_interface_delete(interface);
}

static void test_data_basic_client()
{
	struct mrpc_interface *interface;
	struct mrpc_data *data;
	wchar_t *s;
	struct mrpc_wchar_array *wchar_array;
	uint64_t u64_value;
	uint32_t u32_value;
	uint32_t hash_value;

	interface = mrpc_interface_create(data_method_constructors_client);
	ASSERT(interface != NULL, "interface cannot be NULL");

	data = mrpc_data_create(interface, 0);
	ASSERT(data != NULL, "data cannot be NULL");
	u32_value = 123458ul;
	mrpc_data_set_request_param_value(data, 0, &u32_value);
	u64_value = 18889899089089ull;
	mrpc_data_set_request_param_value(data, 1, &u64_value);
	s = (wchar_t *) ff_calloc(11, sizeof(s[0]));
	memcpy(s, L"1234567890", 10 * sizeof(s[0]));
	wchar_array = mrpc_wchar_array_create(s, 10);
	mrpc_data_set_request_param_value(data, 2, wchar_array);
	hash_value = mrpc_data_get_request_hash(data, 12345);
	ASSERT(hash_value == 1821522797ul, "wrong hash value");
	/* mrpc_data_delete() will delete wchar_array, which, in turn, will delete s */
	mrpc_data_delete(data);

	mrpc_interface_delete(interface);
}

static void test_data_basic_server()
{
	struct mrpc_interface *interface;
	struct mrpc_data *data;
	wchar_t *s;
	struct mrpc_wchar_array *wchar_array;
	uint64_t u64_value;
	uint32_t u32_value;

	interface = mrpc_interface_create(data_method_constructors_server);
	ASSERT(interface != NULL, "interface cannot be NULL");

	data = mrpc_data_create(interface, 0);
	ASSERT(data != NULL, "data cannot be NULL");
	u32_value = 123458ul;
	mrpc_data_set_response_param_value(data, 0, &u32_value);
	u64_value = 18889899089089ull;
	mrpc_data_set_response_param_value(data, 1, &u64_value);
	s = (wchar_t *) ff_calloc(11, sizeof(s[0]));
	memcpy(s, L"1234567890", 10 * sizeof(s[0]));
	wchar_array = mrpc_wchar_array_create(s, 10);
	mrpc_data_set_response_param_value(data, 2, wchar_array);
	/* mrpc_data_delete() will delete wchar_array, which, in turn, will delete s */
	mrpc_data_delete(data);

	mrpc_interface_delete(interface);
}

static void test_data_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_data_create_delete();
	test_data_basic_client();
	test_data_basic_server();
	ff_core_shutdown();
}

/* end of mrpc_data tests */
#pragma endregion

#pragma region mrpc_client and mrpc_server tests

static void test_client_create_delete()
{
	struct mrpc_client *client;

	client = mrpc_client_create();
	ASSERT(client != NULL, "client cannot be NULL");
	mrpc_client_delete(client);
}

static void test_server_create_delete()
{
	struct mrpc_server *server;

	server = mrpc_server_create();
	ASSERT(server != NULL, "server cannot be NULL");
	mrpc_server_delete(server);
}

static void server_method_callback1(struct mrpc_data *data, void *service_ctx)
{
	int32_t *s32_ptr;
	struct mrpc_blob *blob;
	struct mrpc_wchar_array *wchar_array;
	struct mrpc_char_array *char_array;
	struct ff_stream *stream;
	const wchar_t *s;
	char buf[5];
	uint64_t u64_value;
	int len;
	int is_equal;
	enum ff_result result;

	ASSERT(service_ctx == (void *) 1234ul, "unexpected service_ctx value");

	mrpc_data_get_request_param_value(data, 0, (void **) &s32_ptr);
	ASSERT(*s32_ptr == -5433734, "unexpected value");

	mrpc_data_get_request_param_value(data, 1, (void **) &blob);
	len = mrpc_blob_get_size(blob);
	ASSERT(len == 5, "wrong blob size");
	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	ASSERT(stream != NULL, "cannot open blob for reading");
	result = ff_stream_read(stream, buf, 5);
	ASSERT(result == FF_SUCCESS, "cannot read from stream");
	is_equal = (memcmp(buf, "12345", 5) == 0);
	ASSERT(is_equal, "unexpected value received from the stream");
	ff_stream_delete(stream);

	mrpc_data_get_request_param_value(data, 2, (void **) &wchar_array);
	len = mrpc_wchar_array_get_len(wchar_array);
	ASSERT(len == 6, "unexpected value received from the stream");
	s = mrpc_wchar_array_get_value(wchar_array);
	is_equal = (memcmp(s, L"987654", 6 * sizeof(s[0])) == 0);
	ASSERT(is_equal, "unexpected value received from the stream");

	memcpy(buf, "foo", 3 * sizeof(buf[0]));
	char_array = mrpc_char_array_create(buf, 3);
	mrpc_data_set_response_param_value(data, 0, char_array);

	u64_value = 7367289343278ull;
	mrpc_data_set_response_param_value(data, 1, &u64_value);
}

static void server_method_callback2(struct mrpc_data *data, void *service_ctx)
{
	uint32_t u32_value;

	u32_value = 5728933ul;
	mrpc_data_set_response_param_value(data, 0, &u32_value);
}

static struct mrpc_method *server_method_constructor1()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_int32_param_create,
		mrpc_blob_param_create,
		mrpc_wchar_array_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_char_array_param_create,
		mrpc_uint64_param_create,
		NULL,
	};

	method = mrpc_method_create_server_method(request_param_constructors, response_param_constructors, server_method_callback1);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static struct mrpc_method *server_method_constructor2()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_uint32_param_create,
		NULL,
	};

	method = mrpc_method_create_server_method(request_param_constructors, response_param_constructors, server_method_callback2);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static const mrpc_method_constructor server_method_constructors[] =
{
	server_method_constructor1,
	server_method_constructor2,
	NULL,
};

static struct mrpc_method *client_method_constructor1()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		mrpc_int32_param_create,
		mrpc_blob_param_create,
		mrpc_wchar_array_param_create,
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_char_array_param_create,
		mrpc_uint64_param_create,
		NULL,
	};
	static const int is_key[] =
	{
		1,
		1,
		0,
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static struct mrpc_method *client_method_constructor2()
{
	struct mrpc_method *method;
	static const mrpc_param_constructor request_param_constructors[] =
	{
		NULL,
	};
	static const mrpc_param_constructor response_param_constructors[] =
	{
		mrpc_uint32_param_create,
		NULL,
	};
	static const int is_key[] =
	{
		0, /* fake parameter */
	};

	method = mrpc_method_create_client_method(request_param_constructors, response_param_constructors, is_key);
	ASSERT(method != NULL, "unexpected value returned");
	return method;
}

static const mrpc_method_constructor client_method_constructors[] =
{
	client_method_constructor1,
	client_method_constructor2,
	NULL,
};

static void test_client_start_stop()
{
	struct mrpc_client *client;
	struct ff_stream_connector *stream_connector;
	struct ff_arch_net_addr *addr;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8593);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();
	mrpc_client_start(client, stream_connector);
	mrpc_client_stop(client);
	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);
}

static void test_client_start_stop_multiple()
{
	struct mrpc_client *client;
	struct ff_stream_connector *stream_connector;
	struct ff_arch_net_addr *addr;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8594);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();
	mrpc_client_start(client, stream_connector);
	ff_core_sleep(100);
	mrpc_client_stop(client);
	for (i = 0; i < 10; i++)
	{
		mrpc_client_start(client, stream_connector);
		mrpc_client_stop(client);
	}
	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);
}

static void test_server_start_stop()
{
	struct mrpc_server *server;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	enum ff_result result;

	service_interface = mrpc_interface_create(server_method_constructors);
	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8595);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create();
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, service_interface, service_ctx, stream_acceptor);
	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
	mrpc_interface_delete(service_interface);
}

static void test_server_start_stop_multiple()
{
	struct mrpc_server *server;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	int i;
	enum ff_result result;

	service_interface = mrpc_interface_create(server_method_constructors);
	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8596);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create();
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, service_interface, service_ctx, stream_acceptor);
	ff_core_sleep(100);
	mrpc_server_stop(server);
	for (i = 0; i < 10; i++)
	{
		mrpc_server_start(server, service_interface, service_ctx, stream_acceptor);
		mrpc_server_stop(server);
	}
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
	mrpc_interface_delete(service_interface);
}

static void server_accept_fiberpool_func(void *ctx)
{
	struct ff_event *event;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct ff_stream *streams[10];
	int i;
	enum ff_result result;

	event = (struct ff_event *) ctx;
	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8597);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(stream_connector);
	for (i = 0; i < 10; i++)
	{
		struct ff_stream *stream;

		stream = ff_stream_connector_connect(stream_connector);
		ASSERT(stream != NULL, "stream cannot be NULL");
		streams[i] = stream;
	}
	ff_core_sleep(100);
	for (i = 0; i < 10; i++)
	{
		struct ff_stream *stream;

		stream = streams[i];
		ff_stream_delete(stream);
	}
	ff_stream_connector_shutdown(stream_connector);
	ff_stream_connector_delete(stream_connector);
	ff_event_set(event);
}

static void test_server_accept()
{
	struct mrpc_server *server;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	struct ff_event *event;
	enum ff_result result;

	service_interface = mrpc_interface_create(server_method_constructors);
	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8597);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create();
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, service_interface, service_ctx, stream_acceptor);

	event = ff_event_create(FF_EVENT_AUTO);
	ff_core_fiberpool_execute_async(server_accept_fiberpool_func, event);
	ff_event_wait(event);
	ff_event_delete(event);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
	mrpc_interface_delete(service_interface);
}

static void test_client_server_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_client_create_delete();
	test_server_create_delete();
	test_client_start_stop();
	test_client_start_stop_multiple();
	test_server_start_stop();
	test_server_start_stop_multiple();
	test_server_accept();
	ff_core_shutdown();
}

/* end of mrpc_client and mrpc_server tests */
#pragma endregion

static void test_all()
{
	test_char_array_all();
	test_wchar_array_all();
	test_blob_all();
	test_int_param_all();
	test_char_array_param_all();
	test_wchar_array_param_all();
	test_blob_param_all();
	test_method_all();
	test_interface_all();
	test_data_all();
	test_client_server_all();
}

int main(int argc, char* argv[])
{
	test_all();
	printf("ALL TESTS PASSED\n");

	return 0;
}
