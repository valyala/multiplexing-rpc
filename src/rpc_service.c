interface foo
{
	method bar
	{
		request
		{
			uint32 a
			int64 b
			blob c
		}
		response
		{
			int32 d
		}
	}

	method baz
	{
		request
		{
		}
		response
		{
		}
	}
}

INTERFACE ::= "interface" id "{" METHODS_LIST "}"
METHODS_LIST ::= METHOD { METHOD }
METHOD ::= "method" id "{" REQUEST_PARAMS RESPONSE_PARAMS "}"
REQUEST_PARAMS ::= "request" "{" PARAMS_LIST "}"
RESPONSE_PARAMS ::= "response" "{" PARAMS_LIST "}"
PARAMS_LIST ::= PARAM { PARAM }
PARAM ::= TYPE id
TYPE ::= "uint32" | "uint64" | "int32" | "int64" | "string" | "blob"

id = [a-z_][a-z_\d]*

#define MAX_REQUEST_PROCESSORS_CNT (UCHAR_MAX + 1)
#define MAX_WRITER_QUEUE_SIZE 1000
#define MAX_PACKETS_CNT 1000

struct mrpc_client_stream_processor
{
	struct ff_blocking_queue *writer_queue;
	struct ff_pool *request_processors_pool;
	struct ff_event *request_processors_stop_event;
	struct mrpc_bitmap *request_processors_bitmap;
	struct ff_pool *packets_pool;
	struct mrpc_client_request_processor *request_processors[MAX_REQUEST_PROCESSORS_CNT];
	struct ff_stream *stream;
	int request_processors_cnt;
};

static void skip_writer_queue_packets(struct mrpc_client_stream_processor *stream_processor)
{
	for (;;)
	{
		struct mrpc_packet *packet;

		ff_blocking_queue_get(stream_processor->writer_queue, &packet);
		if (packet == NULL)
		{
			int is_empty;

			is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
			ff_assert(is_empty);
			break;
		}
		ff_pool_release_entry(stream_processor->packets_pool, packet);
	}
}

static void stream_writer_func(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;

	for (;;)
	{
		struct mrpc_packet *packet;
		int is_empty;
		enum ff_result result;

		ff_blocking_queue_get(stream_processor->writer_queue, &packet);
		if (packet == NULL)
		{
			is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
			ff_assert(is_empty);
			break;
		}
		result = mrpc_packet_write_to_stream(packet, stream_processor->stream);
		ff_pool_release_entry(stream_processor->packets_pool, packet);
		is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
		if (result == FF_SUCCESS && is_empty)
		{
			result = ff_stream_flush(stream_processor->stream);
		}
		if (result != FF_SUCCESS)
		{
			mrpc_client_stream_processor_stop_async(stream_processor);
			skip_writer_queue_packets(stream_processor);
			break;
		}
	}

	ff_event_set(stream_processor->writer_stop_event);
}

static struct mrpc_packet *acquire_packet(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_packet *packet;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
	return packet;
}

static void release_packet(void *ctx, struct mrpc_packet *packet)
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	mrpc_packet_reset(packet);
	ff_pool_release_entry(stream_processor->packets_pool, packet);
}

static uint8_t acquire_request_id(struct mrpc_client_stream_processor *stream_processor)
{
	int request_id;

	request_id = mrpc_bitmap_acquire_bit(stream_processor->request_processors_bitmap);
	ff_assert(request_id >= 0);
	ff_assert(request_id < MAX_REQUEST_PROCESSORS_CNT);

	return (uint8_t) request_id;
}

static void release_request_id(void *ctx, uint8_t request_id)
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	mrpc_bitmap_release_bit(stream_processor->request_processors_bitmap, request_id);
}

static void *create_request_processor(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_client_request_processor *request_processor;
	uint8_t request_id;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;

	request_id = acquire_request_id(stream_processor);
	request_processor = mrpc_client_request_processor_create(acquire_packet, release_packet, stream_processor, release_request_id, stream_processor,
		stream_processor->writer_queue, request_id);
	return request_processor;
}

static void delete_request_processor(void *ctx)
{
	struct mrpc_client_request_processor *request_processor;

	request_processor = (struct mrpc_client_request_processor *) ctx;
	mrpc_client_request_processor_delete(request_processor);
}

static struct mrpc_client_request_processor *acquire_request_processor(struct mrpc_client_stream_processor *stream_processor)
{
	struct mrpc_client_request_processor *request_processor;
	uint8_t request_id;

	ff_assert(stream_processor->request_processors_cnt >= 0);
	ff_assert(stream_processor->request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);

	request_processor = (struct mrpc_client_request_processor *) ff_pool_acquire_entry(stream_processor->request_processors_pool);
	request_id = mrpc_client_request_processor_get_request_id(request_processor);
	ff_assert(stream_processor->request_processors[request_id] == NULL);
	stream_processor->request_processors[request_id] = request_processor;

	stream_processor->request_processors_cnt++;
	ff_assert(stream_processor->request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	if (stream_processor->request_processors_cnt == 1)
	{
		ff_event_reset(stream_processor->request_processors_stop_event);
	}

	return request_processor;
}

static void release_request_processor(struct mrpc_client_stream_processor *stream_processor, struct mrpc_client_request_processor *request_processor)
{
	uint8_t request_id;

	ff_assert(stream_processor->request_processors_cnt > 0);
	ff_assert(stream_processor->request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);

	request_id = mrpc_client_request_processor_get_request_id(request_processor);
	ff_assert(stream_processor->request_processors[request_id] == request_processor);
	stream_processor->request_processors[request_id] = NULL;
	ff_pool_release_entry(stream_processor->request_processors_pool, request_processor);

	stream_processor->request_processors_cnt--;
	if (stream_processor->request_processors_cnt == 0)
	{
		ff_event_set(stream_processor->request_processors_stop_event);
	}
}

static void *create_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = mrpc_packet_create();
	return packet;
}

static void delete_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = (struct mrpc_packet *) ctx;
	mrpc_packet_delete(packet);
}

static void stop_request_processor(void *entry, void *ctx, int is_acquired)
{
	struct mrpc_client_request_processor *request_processor;

	request_processor = (struct mrpc_client_request_processor *) entry;
	if (is_acquired)
	{
		mrpc_client_request_processor_stop_async(request_processor);
	}
}

static void stop_all_request_processors(struct mrpc_client_stream_processor *stream_processor)
{
	ff_pool_for_each_entry(stream_processor->request_processors_pool, stop_request_processor, stream_processor);
	ff_event_wait(stream_processor->request_processors_stop_event);
	ff_assert(stream_processor->request_processors_cnt == 0);
}

static void start_stream_writer(struct mrpc_client_stream_processor *stream_processor)
{
	ff_core_fiberpool_execute_async(stream_writer_func, stream_processor);
}

static void stop_stream_writer(struct mrpc_client_stream_processor *stream_processor)
{
	ff_blocking_queue_put(stream_processor->writer_queue, NULL);
	ff_event_wait(stream_processor->writer_stop_event);
}

struct mrpc_client_stream_processor *mrpc_client_stream_processor_create()
{
	struct mrpc_client_stream_processor *stream_processor;
	int i;

	stream_processor = (struct mrpc_client_stream_processor *) ff_malloc(sizeof(*stream_processor));
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_WRITER_QUEUE_SIZE);
	stream_processor->request_processors_pool = ff_pool_create(MAX_REQUEST_PROCESSORS_CNT, create_request_processor, stream_processor, delete_request_processor);
	stream_processor->request_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_processors_bitmap = mrpc_bitmap_create(MAX_REQUEST_PROCESSORS_CNT);
	stream_processor->writer_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);

	for (i = 0; i < MAX_REQUEST_PROCESSORS_CNT; i++)
	{
		stream_processor->request_processors[i] = NULL;
	}

	stream_processor->stream = NULL;
	stream_processor->request_processors_cnt = 0;

	return stream_processor;
}

void mrpc_client_stream_processor_delete(struct mrpc_client_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->request_processors_cnt == 0);

	ff_pool_delete(stream_processor->packets_pool);
	ff_event_delete(stream_processor->writer_stop_event);
	mrpc_bitmap_delete(stream_processor->request_processors_bitmap);
	ff_event_delete(stream_processor->request_processors_stop_event);
	ff_pool_delete(stream_processor->request_processors_pool);
	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_free(stream_processor);
}

void mrpc_client_stream_processor_process_stream(struct mrpc_client_stream_processor *stream_processor, struct ff_stream *stream)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->request_processors_cnt == 0);

	stream_processor->stream = stream;
	start_stream_writer(stream_processor);
	ff_event_set(stream_processor->request_processors_stop_event);

	for (;;)
	{
		struct mrpc_packet *packet;
		struct mrpc_client_request_processor *request_processor;
		uint8_t request_id;
		enum ff_result result;

		packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
		result = mrpc_packet_read_from_stream(packet, stream);
		if (result != FF_SUCCESS)
		{
			ff_pool_release_entry(stream_processor->packets_pool, packet);
			break;
		}

		request_id = mrpc_packet_get_request_id(packet);
		request_processor = stream_processor->request_processors[request_id];
		if (request_processor == NULL)
		{
			ff_pool_release_entry(stream_processor->packets_pool, packet);
			break;
		}
		mrpc_client_request_processor_push_packet(request_processor, packet);
	}

	mrpc_client_stream_processor_stop_async(stream_processor);
	stop_all_request_processors(stream_processor);
	ff_assert(stream_processor->request_processors_cnt == 0);
	stop_stream_writer(stream_processor);
}

void mrpc_client_stream_processor_stop_async(struct mrpc_client_stream_processor *stream_processor)
{
	if (stream_processor->stream != NULL)
	{
		ff_stream_disconnect(stream_processor->stream);
		stream_processor->stream = NULL;
	}
}

enum ff_result mrpc_client_stream_processor_invoke_rpc(struct mrpc_client_stream_processor *stream_processor, struct mrpc_data *data)
{
	enum ff_result result = FF_FAILURE;

	if (stream_processor->stream != NULL)
	{
		struct mrpc_client_request_processor *request_processor;

		request_processor = acquire_request_processor(stream_processor);
		result = mrpc_client_request_processor_invoke_rpc(request_processor, data);
		release_request_processor(stream_processor, request_processor);
	}

	return result;
}

#define RECONNECT_TIMEOUT 100
#define MAX_RPC_TRIES_CNT 3
#define RPC_RETRY_TIMEOUT 200

struct mrpc_client
{
	struct ff_stream_connector *stream_connector;
	struct ff_event *stop_event;
	struct ff_event *must_shutdown_event;
	struct mrpc_client_stream_processor *stream_processor;
};

static void main_client_func(void *ctx)
{
	struct mrpc_client *client;

	client = (struct mrpc_client *) ctx;
	for (;;)
	{
		struct ff_stream *stream;

		stream = ff_stream_connector_connect(client->stream_connector);
		if (stream != NULL)
		{
			mrpc_client_stream_processor_process_stream(client->stream_processor, stream);
			ff_stream_delete(stream);
		}
		else
		{
			ff_log_warning(L"cannot establish connection to the stream server");
		}

		result = ff_event_wait_with_timeout(client->must_shutdown_event, RECONNECT_TIMEOUT);
		if (result == FF_SUCCESS)
		{
			/* the mrpc_client_delete() was called */
			break;
		}
	}
	ff_event_set(client->stop_event);
}

struct mrpc_client *mrpc_client_create()
{
	struct mrpc_client *client;

	client = (struct mrpc_client *) ff_malloc(sizeof(*client));
	client->stream_connector = NULL;
	client->stop_event = ff_event_create(FF_EVENT_AUTO);
	client->must_shutdown_event = ff_event_create(FF_EVENT_MANUAL);
	client->stream_processor = mrpc_client_stream_processor_create();

	return client;
}

void mrpc_client_delete(struct mrpc_client *client)
{
	ff_assert(client->stream_connector == NULL);

	mrpc_client_stream_processor_delete(client->stream_processor);
	ff_event_delete(client->must_shutdown_event);
	ff_event_delete(client->stop_event);
	ff_free(client);
}

void mrpc_client_start(struct mrpc_client *client, struct ff_stream_connector *stream_connector)
{
	ff_assert(client->stream_connector == NULL);

	ff_assert(stream_connector != NULL);

	client->stream_connector = stream_connector;
	ff_event_reset(client->must_shutdown_event);
	ff_core_fiberpool_execute_async(main_client_func, client);
}

void mrpc_client_stop(struct mrpc_client *client)
{
	ff_assert(client->stream_connector != NULL);

	ff_event_set(client->must_shutdown_event);
	mrpc_client_stream_processor_stop_async(client->stream_processor);
	ff_event_wait(client->stop_event);

	client->stream_connector = NULL;
}

enum ff_result mrpc_client_invoke_rpc(struct mrpc_client *client, struct mrpc_data *data)
{
	enum ff_result result;
	int tries_cnt = 0;

	for (;;)
	{
		tries_cnt++;
		result = mrpc_client_stream_processor_invoke_rpc(client->stream_processor, data);
		if (result == FF_SUCCESS)
		{
			break;
		}
		if (tries_cnt >= MAX_RPC_TRIES_CNT)
		{
			break;
		}
		result = ff_event_wait_with_timeout(client->must_shutdown_event, RPC_RETRY_TIMEOUT);
		if (result == FF_SUCCESS)
		{
			/* mrpc_client_stop() was called */
			result = FF_FAILURE;
			break;
		}
	}
	return result;
}

foo_interface = foo_interface_create();
service_ctx = foo_service_create();
endpint = ff_endpoint_tcp_create(addr);

server = mrpc_server_create();
mrpc_server_start(server, foo_interface, service_ctx, endpoint);
wait();
mrpc_server_stop(server);
mrpc_server_delete(server);

ff_endpoint_delete(endpoint);
foo_service_delete(service_ctx);
mrpc_interface_delete(foo_interface);


static const mrpc_param_constructor foo_request_param_constructors[] =
{
	mrpc_uint32_param_create,
	mrpc_int64_param_create,
	mrpc_blob_param_create,
	NULL
};

static const mrpc_param_constructor foo_response_param_constructors[] =
{
	mrpc_int32_param_create,
	NULL
};

static void foo_callback(struct mrpc_data *data, void *service_ctx)
{
	uint32_t a;
	int64_t b;
	struct mrpc_blob *c;
	int32_t d;

	mrpc_data_get_request_param_value(data, 0, &a);
	mrpc_data_get_request_param_value(data, 1, &b);
	mrpc_data_get_request_param_value(data, 2, &c);

	service_func_foo(service_ctx, a, b, c, &d);

	mrpc_data_set_response_param_value(data, 0, &d);
}

static struct mrpc_method *create_foo_method()
{
	struct mrpc_method *method;

	method = mrpc_method_create_server(foo_request_param_constructors, foo_response_param_constructors, foo_callback);
	return method;
}

static const mrpc_method_constructor foo_method_constructors[] =
{
	create_foo_method,
	NULL
}

struct mrpc_interface *foo_interface_create()
{
	struct mrpc_interface *interface;

	interface = mrpc_interface_create(foo_method_constructors);
	return interface;
}
