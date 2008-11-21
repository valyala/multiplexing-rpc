#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_int.h"
#include "mrpc/mrpc_char_array.h"
#include "mrpc/mrpc_wchar_array.h"
#include "mrpc/mrpc_blob.h"
#include "mrpc/mrpc_client.h"
#include "mrpc/mrpc_server.h"
#include "mrpc/mrpc_server_stream_handler.h"

#include "ff/ff_core.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"
#include "ff/ff_stream_connector_tcp.h"
#include "ff/ff_stream_acceptor_tcp.h"
#include "ff/arch/ff_arch_net_addr.h"
#include "ff/arch/ff_arch_misc.h"

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

/* start of mrpc_int tests */

static void test_int_hash()
{
	uint32_t u32;
	int32_t s32;
	uint64_t u64;
	int64_t s64;
	uint32_t hash_value;

	u32 = 88928379ul;
	hash_value = mrpc_uint32_get_hash(u32, 1234);
	ASSERT(hash_value == 579656410ul, "unexpected hash value");
	s32 = -88928379l;
	hash_value = mrpc_int32_get_hash(s32, 1234);
	ASSERT(hash_value == 2410314835ul, "unexpected hash value");
	u64 = 887990908928379ull;
	hash_value = mrpc_uint64_get_hash(u64, 1234);
	ASSERT(hash_value == 3339974916ul, "unexpected hash value");
	s64 = -434328892837989ll;
	hash_value = mrpc_int64_get_hash(s64, 1234);
	ASSERT(hash_value == 2931806538ul, "unexpected hash value");
}

struct int_serialization_data
{
	struct ff_event *event;
};

static void int_serialization_fiberpool_func(void *ctx)
{
	struct int_serialization_data *data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	uint32_t u32;
	int32_t s32;
	uint64_t u64;
	int64_t s64;
	enum ff_result result;

	data = (struct int_serialization_data *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4000);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	stream = ff_stream_acceptor_accept(stream_acceptor);
	ASSERT(stream != NULL, "cannot accept connection");

	result = mrpc_uint32_unserialize(&u32, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize u32");
	ASSERT(u32 == 231898ul, "unexpected u32 value unserialized");
	result = mrpc_int32_unserialize(&s32, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize s32");
	ASSERT(s32 == -3432l, "unexpected s32 value unserialized");
	result = mrpc_uint64_unserialize(&u64, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize u64");
	ASSERT(u64 == 3289088989923ull, "unexpected u64 value unserialized");
	result = mrpc_int64_unserialize(&s64, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize s64");
	ASSERT(s64 == -2328943437878732ll, "unexpected s64 value unserialized");

	ff_stream_delete(stream);
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

	ff_event_set(data->event);
}

static void test_int_serialization()
{
	struct int_serialization_data data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct ff_stream *stream;
	uint32_t u32;
	int32_t s32;
	uint64_t u64;
	int64_t s64;
	enum ff_result result;

	data.event = ff_event_create(FF_EVENT_MANUAL);
	ff_core_fiberpool_execute_async(int_serialization_fiberpool_func, &data);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4000);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(stream_connector);
	stream = ff_stream_connector_connect(stream_connector);
	ASSERT(stream != NULL, "cannot connect to server");

	u32 = 231898ul;
	result = mrpc_uint32_serialize(u32, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u32");
	s32 = -3432l;
	result = mrpc_int32_serialize(s32, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize s32");
	u64 = 3289088989923ull;
	result = mrpc_uint64_serialize(u64, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u64");
	s64 = -2328943437878732ll;
	result = mrpc_int64_serialize(s64, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize s64");

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(stream_connector);
	ff_stream_connector_delete(stream_connector);

	ff_event_wait(data.event);
	ff_event_delete(data.event);
}

static void test_int_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_int_hash();
	test_int_serialization();
	ff_core_shutdown();
}

/* end of mrpc_int tests */

/* start of mrpc_char_array tests */

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

struct char_array_serialization_data
{
	struct ff_event *event;
};

static void char_array_serialization_fiberpool_func(void *ctx)
{
	struct char_array_serialization_data *data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_char_array *char_array;
	const char *s;
	int len;
	int is_equal;
	enum ff_result result;

	data = (struct char_array_serialization_data *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4001);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	stream = ff_stream_acceptor_accept(stream_acceptor);
	ASSERT(stream != NULL, "cannot accept connection");

	result = mrpc_char_array_unserialize(&char_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize char_array");
	len = mrpc_char_array_get_len(char_array);
	ASSERT(len == 10, "unexpected length of the char array");
	s = mrpc_char_array_get_value(char_array);
	is_equal = (memcmp(s, "0123456789", 10 * sizeof(s[0])) == 0);
	ASSERT(is_equal, "unexpected value received");
	mrpc_char_array_dec_ref(char_array);

	ff_stream_delete(stream);
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

	ff_event_set(data->event);
}

static void test_char_array_serialization()
{
	struct char_array_serialization_data data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct ff_stream *stream;
	struct mrpc_char_array *char_array;
	char *s;
	enum ff_result result;

	data.event = ff_event_create(FF_EVENT_MANUAL);
	ff_core_fiberpool_execute_async(char_array_serialization_fiberpool_func, &data);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4001);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(stream_connector);
	stream = ff_stream_connector_connect(stream_connector);
	ASSERT(stream != NULL, "cannot connect to server");

	s = (char *) ff_calloc(10, sizeof(s[0]));
	memcpy(s, "0123456789", 10 * sizeof(s[0]));
	char_array = mrpc_char_array_create(s, 10);
	result = mrpc_char_array_serialize(char_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize char array");
	mrpc_char_array_dec_ref(char_array);

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(stream_connector);
	ff_stream_connector_delete(stream_connector);

	ff_event_wait(data.event);
	ff_event_delete(data.event);
}

static void test_char_array_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_char_array_create_delete();
	test_char_array_basic();
	test_char_array_serialization();
	ff_core_shutdown();
}

/* end of mrpc_char_array tests */

/* start of mrpc_wchar_array tests */

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

struct wchar_array_serialization_data
{
	struct ff_event *event;
};

static void wchar_array_serialization_fiberpool_func(void *ctx)
{
	struct wchar_array_serialization_data *data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_wchar_array *wchar_array;
	const wchar_t *s;
	int len;
	int is_equal;
	enum ff_result result;

	data = (struct wchar_array_serialization_data *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4001);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	stream = ff_stream_acceptor_accept(stream_acceptor);
	ASSERT(stream != NULL, "cannot accept connection");

	result = mrpc_wchar_array_unserialize(&wchar_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize wchar_array");
	len = mrpc_wchar_array_get_len(wchar_array);
	ASSERT(len == 10, "unexpected length of the wchar array");
	s = mrpc_wchar_array_get_value(wchar_array);
	is_equal = (memcmp(s, L"0123456789", 10 * sizeof(s[0])) == 0);
	ASSERT(is_equal, "unexpected value received");
	mrpc_wchar_array_dec_ref(wchar_array);

	ff_stream_delete(stream);
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

	ff_event_set(data->event);
}

static void test_wchar_array_serialization()
{
	struct wchar_array_serialization_data data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct ff_stream *stream;
	struct mrpc_wchar_array *wchar_array;
	wchar_t *s;
	enum ff_result result;

	data.event = ff_event_create(FF_EVENT_MANUAL);
	ff_core_fiberpool_execute_async(wchar_array_serialization_fiberpool_func, &data);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4001);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(stream_connector);
	stream = ff_stream_connector_connect(stream_connector);
	ASSERT(stream != NULL, "cannot connect to server");

	s = (wchar_t *) ff_calloc(10, sizeof(s[0]));
	memcpy(s, L"0123456789", 10 * sizeof(s[0]));
	wchar_array = mrpc_wchar_array_create(s, 10);
	result = mrpc_wchar_array_serialize(wchar_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize wchar array");
	mrpc_wchar_array_dec_ref(wchar_array);

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(stream_connector);
	ff_stream_connector_delete(stream_connector);

	ff_event_wait(data.event);
	ff_event_delete(data.event);
}

static void test_wchar_array_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_wchar_array_create_delete();
	test_wchar_array_basic();
	test_wchar_array_serialization();
	ff_core_shutdown();
}

/* end of mrpc_char_array tests */


/* start of  mrpc_blob tests */

static void test_blob_create_delete()
{
	struct mrpc_blob *blob;
	int len;

	blob = mrpc_blob_create(10);
	ASSERT(blob != NULL, "blob cannot be null");
	len = mrpc_blob_get_len(blob);
	ASSERT(len == 10, "unexpected size for the blob");
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

	result = mrpc_blob_get_hash(blob, 432, &hash_value);
	ASSERT(result == FF_SUCCESS, "cannot calculate hash for the blob");
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

struct blob_serialization_data
{
	struct ff_event *event;
};

static void blob_serialization_fiberpool_func(void *ctx)
{
	struct blob_serialization_data *data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_stream *stream;
	struct mrpc_blob *blob;
	struct ff_stream *blob_stream;
	char buf[10];
	int len;
	int is_equal;
	enum ff_result result;

	data = (struct blob_serialization_data *) ctx;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4002);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	ff_stream_acceptor_initialize(stream_acceptor);
	stream = ff_stream_acceptor_accept(stream_acceptor);
	ASSERT(stream != NULL, "cannot accept connection");

	result = mrpc_blob_unserialize(&blob, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize blob");
	len = mrpc_blob_get_len(blob);
	ASSERT(len == 10, "unexpected blob length");
	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	ASSERT(blob_stream != NULL, "cannot open blob stream");
	result = ff_stream_read(blob_stream, buf, 10);
	ASSERT(result == FF_SUCCESS, "cannot read data from blob stream");
	is_equal = (memcmp(buf, "0123456789", 10) == 0);
	ASSERT(is_equal, "unexpected value of the blob");
	ff_stream_delete(blob_stream);
	mrpc_blob_dec_ref(blob);

	ff_stream_delete(stream);
	ff_stream_acceptor_shutdown(stream_acceptor);
	ff_stream_acceptor_delete(stream_acceptor);

	ff_event_set(data->event);
}

static void test_blob_serialization()
{
	struct blob_serialization_data data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct ff_stream *stream;
	struct mrpc_blob *blob;
	struct ff_stream *blob_stream;
	enum ff_result result;

	data.event = ff_event_create(FF_EVENT_MANUAL);
	ff_core_fiberpool_execute_async(blob_serialization_fiberpool_func, &data);

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 4002);
	ASSERT(result == FF_SUCCESS, "cannot resolve localhost address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	ff_stream_connector_initialize(stream_connector);
	stream = ff_stream_connector_connect(stream_connector);
	ASSERT(stream != NULL, "cannot connect to server");

	blob = mrpc_blob_create(10);
	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
	ASSERT(blob_stream != NULL, "cannot open blob stream for writing");
	result = ff_stream_write(blob_stream, "0123456789", 10);
	ASSERT(result == FF_SUCCESS, "cannot write data to blob stream");
	result = ff_stream_flush(blob_stream);
	ASSERT(result == FF_SUCCESS, "cannot flush data to blob stream");
	ff_stream_delete(blob_stream);

	result = mrpc_blob_serialize(blob, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize blob");
	mrpc_blob_dec_ref(blob);

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	ff_stream_delete(stream);
	ff_stream_connector_shutdown(stream_connector);
	ff_stream_connector_delete(stream_connector);

	ff_event_wait(data.event);
	ff_event_delete(data.event);
}

static void test_blob_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_blob_create_delete();
	test_blob_basic();
	test_blob_multiple_read();
	test_blob_ref_cnt();
	test_blob_serialization();
	ff_core_shutdown();
}

/* end of mrpc_blob tests */


/* start of mrpc_client and mrpc_server tests */

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

	server = mrpc_server_create(10);
	ASSERT(server != NULL, "server cannot be NULL");
	mrpc_server_delete(server);
}

static enum ff_result server_method_callback1(struct ff_stream *stream, void *service_ctx)
{
	int32_t s32;
	struct mrpc_blob *blob;
	struct mrpc_wchar_array *wchar_array;
	struct mrpc_char_array *char_array;
	struct ff_stream *blob_stream;
	const wchar_t *ws;
	char *s;
	char buf[5];
	uint64_t u64;
	int len;
	int is_equal;
	enum ff_result result;

	ASSERT(service_ctx == (void *) 1234ul, "unexpected service_ctx value");

	result = mrpc_int32_unserialize(&s32, stream);
	ASSERT(result == FF_SUCCESS, "cannot read s32");
	ASSERT(s32 == - 5433734l, "unexpected value");

	result = mrpc_blob_unserialize(&blob, stream);
	ASSERT(result == FF_SUCCESS, "cannot read blob");
	len = mrpc_blob_get_len(blob);
	ASSERT(len == 5, "wrong blob size");
	blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_READ);
	ASSERT(blob_stream != NULL, "cannot open blob for reading");
	result = ff_stream_read(blob_stream, buf, 5);
	ASSERT(result == FF_SUCCESS, "cannot read from stream");
	is_equal = (memcmp(buf, "12345", 5) == 0);
	ASSERT(is_equal, "unexpected value received from the stream");
	ff_stream_delete(blob_stream);
	mrpc_blob_dec_ref(blob);

	result = mrpc_wchar_array_unserialize(&wchar_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot read wchar array");
	len = mrpc_wchar_array_get_len(wchar_array);
	ASSERT(len == 6, "unexpected value received from the stream");
	ws = mrpc_wchar_array_get_value(wchar_array);
	is_equal = (memcmp(ws, L"987654", 6 * sizeof(ws[0])) == 0);
	ASSERT(is_equal, "unexpected value received from the stream");
	mrpc_wchar_array_dec_ref(wchar_array);

	s = (char *) ff_calloc(3, sizeof(s[0]));
	memcpy(s, "foo", 3 * sizeof(s[0]));
	char_array = mrpc_char_array_create(s, 3);
	result = mrpc_char_array_serialize(char_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize char array to the stream");
	mrpc_char_array_dec_ref(char_array);

	u64 = 7367289343278ull;
	result = mrpc_uint64_serialize(u64, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u64");

	return result;
}

static enum ff_result server_method_callback2(struct ff_stream *stream, void *service_ctx)
{
	uint32_t u32;
	enum ff_result result;

	ASSERT(service_ctx == (void *) 1234ul, "unexpected service_ctx value");

	u32 = 5728933ul;
	result = mrpc_uint32_serialize(u32, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u32");

	return result;
}

static enum ff_result server_method_callback3(struct ff_stream *stream, void *service_ctx)
{
	uint8_t empty_byte = 0;
	enum ff_result result;

	ASSERT(service_ctx == (void *) 1234ul, "unexpected service_ctx value");

	/* send an empty response */
	result = ff_stream_write(stream, &empty_byte, 1);
	ASSERT(result == FF_SUCCESS, "cannot send an empty character to stream");

	return result;
}

static enum ff_result server_stream_handler(struct ff_stream *stream, void *service_ctx)
{
	uint8_t method_id;
	enum ff_result result;

	ASSERT(service_ctx == (void *) 1234ul, "unexpected service_ctx value");

	/* read the method id */
	result = ff_stream_read(stream, &method_id, 1);
	ASSERT(result == FF_SUCCESS, "cannot read method_id");
	ASSERT(method_id <= 2, "unexpected method_id read");
	if (method_id == 0)
	{
		result = server_method_callback1(stream, service_ctx);
		ASSERT(result == FF_SUCCESS, "unexpected result");
	}
	else if (method_id == 1)
	{
		result = server_method_callback2(stream, service_ctx);
		ASSERT(result == FF_SUCCESS, "unexpected result");
	}
	else if (method_id == 2)
	{
		result = server_method_callback3(stream, service_ctx);
		ASSERT(result == FF_SUCCESS, "unexpected result");
	}
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");
	return result;
}

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

struct client_multiple_instances_data
{
	struct ff_event *event;
	int port;
	int workers_cnt;
};

static void client_multiple_instances_fiberpool_func(void *ctx)
{
	struct client_multiple_instances_data *data;
	struct mrpc_client *client;
	struct ff_stream_connector *stream_connector;
	struct ff_arch_net_addr *addr;
	int port;
	enum ff_result result;

	data = (struct client_multiple_instances_data *) ctx;
	port = data->port;
	data->port++;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", port);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();

	mrpc_client_start(client, stream_connector);
	ff_core_sleep(100);
	mrpc_client_stop(client);

	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);

	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void test_client_multiple_instances()
{
	struct client_multiple_instances_data data;
	int i;

	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.port = 8500;
	data.workers_cnt = 5;
	for (i = 0; i < 5; i++)
	{
		ff_core_fiberpool_execute_async(client_multiple_instances_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);
}

static void test_server_start_stop()
{
	struct mrpc_server *server;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8595);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(10);
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);
	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

static void test_server_start_stop_multiple()
{
	struct mrpc_server *server;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8596);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);
	ff_core_sleep(100);
	mrpc_server_stop(server);
	for (i = 0; i < 10; i++)
	{
		mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);
		mrpc_server_stop(server);
	}
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

struct server_multiple_instances_data
{
	struct ff_event *event;
	int port;
	int workers_cnt;
};

static void server_multiple_instances_fiberpool_func(void *ctx)
{
	struct server_multiple_instances_data *data;
	void *service_ctx;
	struct mrpc_server *server;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	int port;
	enum ff_result result;

	data = (struct server_multiple_instances_data *) ctx;
	port = data->port;
	data->port++;
	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", port);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	service_ctx = (void *) 1234ul;

	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);
	ff_core_sleep(100);
	mrpc_server_stop(server);

	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);

	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void test_server_multiple_instances()
{
	struct server_multiple_instances_data data;
	int i;

	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.port = 8500;
	data.workers_cnt = 5;
	for (i = 0; i < 5; i++)
	{
		ff_core_fiberpool_execute_async(server_multiple_instances_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);
}

static void test_server_accept()
{
	struct mrpc_server *server;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8597);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);

	{
		struct ff_stream_connector *stream_connector;
		struct ff_stream *streams[10];
		int i;

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
	}

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

struct client_server_connect_data
{
	struct ff_event *event;
	int workers_cnt;
};

static void client_server_connect_fiberpool_func(void *ctx)
{
	struct client_server_connect_data *data;
	struct mrpc_client *client;
	struct ff_stream_connector *stream_connector;
	struct ff_arch_net_addr *addr;
	int i;
	enum ff_result result;

	data = (struct client_server_connect_data *) ctx;
	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8598);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();

	mrpc_client_start(client, stream_connector);
	mrpc_client_stop(client);

	mrpc_client_start(client, stream_connector);
	ff_core_sleep(100);
	mrpc_client_stop(client);

	for (i = 0; i < 5; i++)
	{
		mrpc_client_start(client, stream_connector);
		ff_core_sleep(10);
		mrpc_client_stop(client);
	}

	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);

	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void test_client_server_connect()
{
	struct client_server_connect_data data;
	struct mrpc_server *server;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8598);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);

	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.workers_cnt = 5;
	for (i = 0; i < 5; i++)
	{
		ff_core_fiberpool_execute_async(client_server_connect_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

static void client_server_rpc_client(int port, int iterations_cnt)
{
	struct ff_arch_net_addr *addr;
	struct mrpc_client *client;
	struct ff_stream_connector *stream_connector;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", port);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();
	mrpc_client_start(client, stream_connector);

	for (i = 0; i < iterations_cnt; i++)
	{
		struct ff_stream *stream;
		struct mrpc_blob *blob;
		struct ff_stream *blob_stream;
		struct mrpc_wchar_array *wchar_array;
		struct mrpc_char_array *char_array;
		wchar_t *ws;
		const char *s;
		int len;
		int is_equal;
		int32_t s32;
		uint64_t u64;
		uint32_t u32;
		uint32_t hash_value;
		uint8_t method_id;
		uint8_t empty_byte;

		stream = mrpc_client_create_request_stream(client, 2);
		ASSERT(stream != NULL, "client must be already connected");

		method_id = 0;
		result = ff_stream_write(stream, &method_id, 1);
		ASSERT(result == FF_SUCCESS, "Cannot write method_id to the client");

		s32 = -5433734l;
		result = mrpc_int32_serialize(s32, stream);
		ASSERT(result == FF_SUCCESS, "cannot serialize s32");

		blob = mrpc_blob_create(5);
		blob_stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
		ASSERT(blob_stream != NULL, "cannot open blob for writing");
		result = ff_stream_write(blob_stream, "12345", 5);
		ASSERT(result == FF_SUCCESS, "cannot write to blob stream");
		result = ff_stream_flush(blob_stream);
		ASSERT(result == FF_SUCCESS, "cannot flush the blob stream");
		ff_stream_delete(blob_stream);
		result = mrpc_blob_serialize(blob, stream);
		ASSERT(result == FF_SUCCESS, "cannot serialize blob to stream");

		ws = (wchar_t *) ff_calloc(6, sizeof(ws[0]));
		memcpy(ws, L"987654", 6 * sizeof(ws[0]));
		wchar_array = mrpc_wchar_array_create(ws, 6);
		result = mrpc_wchar_array_serialize(wchar_array, stream);
		ASSERT(result == FF_SUCCESS, "cannot serialize wchar array to stream");
		mrpc_wchar_array_dec_ref(wchar_array);

		hash_value = mrpc_int32_get_hash(s32, 12345);
		result = mrpc_blob_get_hash(blob, hash_value, &hash_value);
		ASSERT(result == FF_SUCCESS, "cannot calculate hash for blob");
		ASSERT(hash_value == 295991475ul, "unexpected hash value");
		mrpc_blob_dec_ref(blob);

		result = ff_stream_flush(stream);
		ASSERT(result == FF_SUCCESS, "cannot flush the stream");

		result = mrpc_char_array_unserialize(&char_array, stream);
		ASSERT(result == FF_SUCCESS, "cannot unserialize char array");
		len = mrpc_char_array_get_len(char_array);
		ASSERT(len == 3, "unexpected length returned");
		s = mrpc_char_array_get_value(char_array);
		is_equal = (memcmp(s, "foo", 3) == 0);
		ASSERT(is_equal, "unexpected value received");
		mrpc_char_array_dec_ref(char_array);

		result = mrpc_uint64_unserialize(&u64, stream);
		ASSERT(result == FF_SUCCESS, "cannot unserialize u64");
		ASSERT(u64 == 7367289343278ull, "unexpected value received");

		ff_stream_delete(stream);

		stream = mrpc_client_create_request_stream(client, 2);
		ASSERT(stream != NULL, "client must return valid stream");
		method_id = 1;
		result = ff_stream_write(stream, &method_id, 1);
		ASSERT(result == FF_SUCCESS, "cannot write method_id to the stream");
		result = ff_stream_flush(stream);
		ASSERT(result == FF_SUCCESS, "cannot flush the stream");
		result = mrpc_uint32_unserialize(&u32, stream);
		ASSERT(result == FF_SUCCESS, "cannot unserialize u32");
		ASSERT(u32 == 5728933ul, "unexpected value received");
		ff_stream_delete(stream);

		stream = mrpc_client_create_request_stream(client, 2);
		ASSERT(stream != NULL, "client must return valid stream");
		method_id = 2;
		result = ff_stream_write(stream, &method_id, 1);
		ASSERT(result == FF_SUCCESS, "cannot write method_id to the stream");
		result = ff_stream_flush(stream);
		ASSERT(result == FF_SUCCESS, "cannot flush the stream");
		result = ff_stream_read(stream, &empty_byte, 1);
		ASSERT(result == FF_SUCCESS, "cannot read an empty byte");
		ASSERT(empty_byte == 0, "unexpected value of empty byte");
		ff_stream_delete(stream);
	}

	mrpc_client_stop(client);
	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);
}

static void test_client_server_rpc()
{
	struct mrpc_server *server;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8599);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);

	client_server_rpc_client(8599, 10);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

struct client_server_rpc_multiple_clients_data
{
	struct ff_event *event;
	int port;
	int workers_cnt;
};

static void client_server_rpc_multiple_clients_fiberpool_func(void *ctx)
{
	struct client_server_rpc_multiple_clients_data *data;

	data = (struct client_server_rpc_multiple_clients_data *) ctx;
	client_server_rpc_client(data->port, 5);
	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void test_client_server_rpc_multiple_clients()
{
	struct client_server_rpc_multiple_clients_data data;
	struct mrpc_server *server;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_arch_net_addr *addr;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 8601);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	service_ctx = (void *) 1234ul;
	mrpc_server_start(server, server_stream_handler, service_ctx, stream_acceptor);
	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.port = 8601;
	data.workers_cnt = 20;
	for (i = 0; i < 20; i++)
	{
		ff_core_fiberpool_execute_async(client_server_rpc_multiple_clients_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

static enum ff_result server_echo_callback(struct ff_stream *stream, void *service_ctx)
{
	uint32_t u32;
	int32_t s32;
	uint64_t u64;
	int64_t s64;
	struct mrpc_char_array *char_array;
	struct mrpc_wchar_array *wchar_array;
	struct mrpc_blob *blob;
	enum ff_result result;

	result = mrpc_uint32_unserialize(&u32, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize u32");
	result = mrpc_int32_unserialize(&s32, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserializes s32");
	result = mrpc_uint64_unserialize(&u64, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize u64");
	result = mrpc_int64_unserialize(&s64, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize s64");
	result = mrpc_char_array_unserialize(&char_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize char array");
	result = mrpc_wchar_array_unserialize(&wchar_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize wchar array");
	result = mrpc_blob_unserialize(&blob, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize blob");

	result = mrpc_uint32_serialize(u32, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u32");
	result = mrpc_int32_serialize(s32, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize s32");
	result = mrpc_uint64_serialize(u64, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u64");
	result = mrpc_int64_serialize(s64, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize s64");
	result = mrpc_char_array_serialize(char_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize char array");
	result = mrpc_wchar_array_serialize(wchar_array, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize wchar array");
	result = mrpc_blob_serialize(blob, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize blob");

	mrpc_char_array_dec_ref(char_array);
	mrpc_wchar_array_dec_ref(wchar_array);
	mrpc_blob_dec_ref(blob);

	return result;
}

static enum ff_result server_echo_stream_handler(struct ff_stream *stream, void *service_ctx)
{
	uint8_t method_id;
	enum ff_result result;

	result = ff_stream_read(stream, &method_id, 1);
	ASSERT(result == FF_SUCCESS, "cannot read method_id");
	ASSERT(method_id == 0, "unexpected method_id");
	result = server_echo_callback(stream, service_ctx);
	ASSERT(result == FF_SUCCESS, "error in echo callback");
	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	return result;
}

static int client_server_echo_get_random_uint(int max_value)
{
	int rnd_value;

	ff_arch_misc_fill_buffer_with_random_data(&rnd_value, sizeof(rnd_value));
	rnd_value &= (1ul << 31) - 1;
	ASSERT(rnd_value >= 0, "unexpected value");
	rnd_value %= max_value;
	return rnd_value;
}

static struct mrpc_char_array *client_server_echo_create_char_array()
{
	struct mrpc_char_array *char_array;
	char *s;
	int len;

	len = client_server_echo_get_random_uint(1 << 14);
	s = (char *) ff_calloc(len, sizeof(s[0]));
	ff_arch_misc_fill_buffer_with_random_data(s, len * sizeof(s[0]));
	char_array = mrpc_char_array_create(s, len);
	return char_array;
}

static struct mrpc_wchar_array *client_server_echo_create_wchar_array()
{
	struct mrpc_wchar_array *wchar_array;
	wchar_t *s;
	int len;

	len = client_server_echo_get_random_uint(1 << 14);
	s = (wchar_t *) ff_calloc(len, sizeof(s[0]));
	ff_arch_misc_fill_buffer_with_random_data(s, len * sizeof(s[0]));
	if (sizeof(wchar_t) == 4)
	{
		/* make all chars less than 0x10000 */
		int i;

		for (i = 0; i < len; i++)
		{
			s[i] = s[i] & 0xffff;
		}
	}
	wchar_array = mrpc_wchar_array_create(s, len);
	return wchar_array;
}

static struct mrpc_blob *client_server_echo_create_blob()
{
	struct mrpc_blob *blob;
	struct ff_stream *stream;
	uint8_t *buf;
	int size;

	size = client_server_echo_get_random_uint(1024 * 1024);
	blob = mrpc_blob_create(size);
	stream = mrpc_blob_open_stream(blob, MRPC_BLOB_WRITE);
	ASSERT(stream != NULL, "cannot open blob stream for writing");
	buf = (uint8_t *) ff_calloc(0x10000, sizeof(buf[0]));
	while (size > 0)
	{
		int len;
		enum ff_result result;

		len = size > 0x10000 ? 0x10000 : size;
		ff_arch_misc_fill_buffer_with_random_data(buf, len);
		result = ff_stream_write(stream, buf, len);
		ASSERT(result == FF_SUCCESS, "cannot write to the blob stream");

		size -= len;
	}
	ff_free(buf);
	ff_stream_flush(stream);
	ff_stream_delete(stream);
	return blob;
}

static void client_server_echo_compare_char_arrays(struct mrpc_char_array *char_array1, struct mrpc_char_array *char_array2)
{
	const char *s1, *s2;
	int len1, len2;
	int is_equal;

	len1 = mrpc_char_array_get_len(char_array1);
	len2 = mrpc_char_array_get_len(char_array2);
	ASSERT(len1 == len2, "wrong lengths");
	s1 = mrpc_char_array_get_value(char_array1);
	s2 = mrpc_char_array_get_value(char_array2);
	is_equal = (memcmp(s1, s2, len1 * sizeof(s1[0])) == 0);
	ASSERT(is_equal, "wrong values");
}

static void client_server_echo_compare_wchar_arrays(struct mrpc_wchar_array *wchar_array1, struct mrpc_wchar_array *wchar_array2)
{
	const wchar_t *s1, *s2;
	int len1, len2;
	int is_equal;

	len1 = mrpc_wchar_array_get_len(wchar_array1);
	len2 = mrpc_wchar_array_get_len(wchar_array2);
	ASSERT(len1 == len2, "wrong lengths");
	s1 = mrpc_wchar_array_get_value(wchar_array1);
	s2 = mrpc_wchar_array_get_value(wchar_array2);
	is_equal = (memcmp(s1, s2, len1 * sizeof(s1[0])) == 0);
	ASSERT(is_equal, "wrong values");
}

static void client_server_echo_compare_blobs(struct mrpc_blob *blob1, struct mrpc_blob *blob2)
{
	struct ff_stream *stream1, *stream2;
	uint8_t *buf1, *buf2;
	int len1, len2;

	len1 = mrpc_blob_get_len(blob1);
	len2 = mrpc_blob_get_len(blob2);
	ASSERT(len1 == len2, "wrong lengths");

	stream1 = mrpc_blob_open_stream(blob1, MRPC_BLOB_READ);
	ASSERT(stream1 != NULL, "cannot open blob stream");
	stream2 = mrpc_blob_open_stream(blob2, MRPC_BLOB_READ);
	ASSERT(stream2 != NULL, "cannot open blob stream");

	buf1 = (uint8_t *) ff_calloc(0x10000, sizeof(buf1[0]));
	buf2 = (uint8_t *) ff_calloc(0x10000, sizeof(buf2[0]));
	while (len1 > 0)
	{
		int len;
		int is_equal;
		enum ff_result result;

		len = len1 > 0x10000 ? 0x10000 : len1;
		result = ff_stream_read(stream1, buf1, len);
		ASSERT(result == FF_SUCCESS, "cannot read from blob stream");
		result = ff_stream_read(stream2, buf2, len);
		ASSERT(result == FF_SUCCESS, "cannot read from blob stream");
		is_equal = (memcmp(buf1, buf2, len) == 0);
		ASSERT(is_equal, "blob values cannot be different");

		len1 -= len;
	}
	ff_free(buf1);
	ff_free(buf2);

	ff_stream_delete(stream1);
	ff_stream_delete(stream2);
}

static void client_server_echo_client_rpc(struct mrpc_client *client)
{
	struct ff_stream *stream;
	struct mrpc_char_array *char_array1, *char_array2;
	struct mrpc_wchar_array *wchar_array1, *wchar_array2;
	struct mrpc_blob *blob1, *blob2;
	uint32_t u32_1, u32_2;
	int32_t s32_1, s32_2;
	uint64_t u64_1, u64_2;
	int64_t s64_1, s64_2;
	uint8_t method_id;
	enum ff_result result;

	stream = mrpc_client_create_request_stream(client, 2);
	ASSERT(stream != NULL, "stream must be created");
	method_id = 0;
	result = ff_stream_write(stream, &method_id, 1);
	ASSERT(result == FF_SUCCESS, "cannot write method_id");

	u32_1 = 2 * client_server_echo_get_random_uint((1ul << 31) - 1);
	s32_1 = 5 - u32_1;
	u64_1 = 23 * (uint64_t) u32_1;
	s64_1 = 43543 - 4534 * (uint64_t) u32_1;
	char_array1 = client_server_echo_create_char_array();
	wchar_array1 = client_server_echo_create_wchar_array();
	blob1 = client_server_echo_create_blob();

	result = mrpc_uint32_serialize(u32_1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u32");
	result = mrpc_int32_serialize(s32_1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize s32");
	result = mrpc_uint64_serialize(u64_1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize u64");
	result = mrpc_int64_serialize(s64_1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize s64");
	result = mrpc_char_array_serialize(char_array1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize char array");
	result = mrpc_wchar_array_serialize(wchar_array1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize wchar array");
	result = mrpc_blob_serialize(blob1, stream);
	ASSERT(result == FF_SUCCESS, "cannot serialize blob");

	result = ff_stream_flush(stream);
	ASSERT(result == FF_SUCCESS, "cannot flush the stream");

	result = mrpc_uint32_unserialize(&u32_2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize u32");
	ASSERT(u32_1 == u32_2, "unexpected value");
	result = mrpc_int32_unserialize(&s32_2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize s32");
	ASSERT(s32_1 == s32_2, "unexpected value");
	result = mrpc_uint64_unserialize(&u64_2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize u64");
	ASSERT(u64_1 == u64_2, "unexpected value");
	result = mrpc_int64_unserialize(&s64_2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize s64");
	ASSERT(s64_1 == s64_2, "unexpected value");
	result = mrpc_char_array_unserialize(&char_array2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize char array");
	client_server_echo_compare_char_arrays(char_array1, char_array2);
	result = mrpc_wchar_array_unserialize(&wchar_array2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize wchar array");
	client_server_echo_compare_wchar_arrays(wchar_array1, wchar_array2);
	result = mrpc_blob_unserialize(&blob2, stream);
	ASSERT(result == FF_SUCCESS, "cannot unserialize blob");
	client_server_echo_compare_blobs(blob1, blob2);

	mrpc_char_array_dec_ref(char_array1);
	mrpc_wchar_array_dec_ref(wchar_array1);
	mrpc_blob_dec_ref(blob1);

	mrpc_char_array_dec_ref(char_array2);
	mrpc_wchar_array_dec_ref(wchar_array2);
	mrpc_blob_dec_ref(blob2);

	ff_stream_delete(stream);
}

static void client_server_echo_client(int port, int iterations_cnt)
{
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct mrpc_client *client;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", port);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();
	mrpc_client_start(client, stream_connector);
	for (i = 0; i < iterations_cnt; i++)
	{
		client_server_echo_client_rpc(client);
	}
	mrpc_client_stop(client);
	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);
}

static void test_client_server_echo_rpc()
{
	void *server_ctx = NULL;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct mrpc_server *server;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 10101);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	mrpc_server_start(server, server_echo_stream_handler, server_ctx, stream_acceptor);

	client_server_echo_client(10101, 5);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

struct client_server_echo_rpc_multiple_clients_data
{
	struct ff_event *event;
	int port;
	int workers_cnt;
};

static void client_server_echo_rpc_multiple_clients_fiberpool_func(void *ctx)
{
	struct client_server_echo_rpc_multiple_clients_data *data;

	data = (struct client_server_echo_rpc_multiple_clients_data *) ctx;

	client_server_echo_client(data->port, 2);

	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void test_client_server_echo_rpc_multiple_clients()
{
	struct client_server_echo_rpc_multiple_clients_data data;
	void *server_ctx = NULL;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct mrpc_server *server;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 10997);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	mrpc_server_start(server, server_echo_stream_handler, server_ctx, stream_acceptor);

	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.port = 10997;
	data.workers_cnt = 5;
	for (i = 0; i < 5; i++)
	{
		ff_core_fiberpool_execute_async(client_server_echo_rpc_multiple_clients_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

struct client_server_echo_rpc_concurrent_data
{
	struct ff_event *event;
	struct mrpc_client *client;
	int workers_cnt;
};

static void client_server_echo_rpc_concurrent_fiberpool_func(void *ctx)
{
	struct client_server_echo_rpc_concurrent_data *data;

	data = (struct client_server_echo_rpc_concurrent_data *) ctx;

	client_server_echo_client_rpc(data->client);

	data->workers_cnt--;
	if (data->workers_cnt == 0)
	{
		ff_event_set(data->event);
	}
}

static void client_server_echo_client_concurrent(int port, int workers_cnt)
{
	struct client_server_echo_rpc_concurrent_data data;
	struct ff_arch_net_addr *addr;
	struct ff_stream_connector *stream_connector;
	struct mrpc_client *client;
	int i;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", port);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_connector = ff_stream_connector_tcp_create(addr);
	client = mrpc_client_create();
	mrpc_client_start(client, stream_connector);

	data.event = ff_event_create(FF_EVENT_MANUAL);
	data.client = client;
	data.workers_cnt = workers_cnt;
	for (i = 0; i < workers_cnt; i++)
	{
		ff_core_fiberpool_execute_async(client_server_echo_rpc_concurrent_fiberpool_func, &data);
	}
	ff_event_wait(data.event);
	ff_event_delete(data.event);

	mrpc_client_stop(client);
	mrpc_client_delete(client);
	ff_stream_connector_delete(stream_connector);
}

static void test_client_server_echo_rpc_concurrent()
{
	void *server_ctx = NULL;
	struct ff_arch_net_addr *addr;
	struct ff_stream_acceptor *stream_acceptor;
	struct mrpc_server *server;
	enum ff_result result;

	addr = ff_arch_net_addr_create();
	result = ff_arch_net_addr_resolve(addr, L"localhost", 10102);
	ASSERT(result == FF_SUCCESS, "cannot resolve local address");
	stream_acceptor = ff_stream_acceptor_tcp_create(addr);
	server = mrpc_server_create(100);
	mrpc_server_start(server, server_echo_stream_handler, server_ctx, stream_acceptor);

	client_server_echo_client_concurrent(10102, 10);

	mrpc_server_stop(server);
	mrpc_server_delete(server);
	ff_stream_acceptor_delete(stream_acceptor);
}

static void test_client_server_all()
{
	ff_core_initialize(LOG_FILENAME);
	test_client_create_delete();
	test_server_create_delete();
	test_client_start_stop();
	test_client_start_stop_multiple();
	test_client_multiple_instances();
	test_server_start_stop();
	test_server_start_stop_multiple();
	test_server_multiple_instances();
	test_server_accept();
	test_client_server_connect();
	test_client_server_rpc();
	test_client_server_rpc_multiple_clients();
	test_client_server_echo_rpc();
	test_client_server_echo_rpc_multiple_clients();
	test_client_server_echo_rpc_concurrent();
	ff_core_shutdown();
}

/* end of mrpc_client and mrpc_server tests */

static void test_all()
{
	test_int_all();
	test_char_array_all();
	test_wchar_array_all();
	test_blob_all();
	test_client_server_all();
}

int main(int argc, char* argv[])
{
	test_all();
	printf("ALL TESTS PASSED\n");

	return 0;
}
