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
