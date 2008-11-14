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
TYPE ::= "uint32" | "uint64" | "int32" | "int64" | "wchar_array" | "char_array" | "blob"

id = [a-z_][a-z_\d]*

foo_interface = foo_interface_create();
service_ctx = foo_service_create();
stream_acceptor = ff_stream_acceptor_tcp_create(addr);

server = mrpc_server_create();
mrpc_server_start(server, foo_interface, service_ctx, stream_acceptor);
wait();
mrpc_server_stop(server);
mrpc_server_delete(server);

ff_stream_acceptor_delete(stream_acceptor);
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

static const struct mrpc_method_server_description foo_method_description =
{
	foo_request_param_constructors,
	foo_response_param_constructors,
	foo_callback
};

static const struct mrpc_method_server_description *foo_method_descriptions[] =
{
	foo_method_description,
	NULL
};

struct mrpc_interface *foo_interface_create()
{
	struct mrpc_interface *interface;

	interface = mrpc_interface_server_create(foo_method_descriptions);
	return interface;
}
