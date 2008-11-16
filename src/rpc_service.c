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

-------------------------------------------------------------------------------
enum param_type
{
	PARAM_UINT32,
	PARAM_INT32,
	PARAM_UINT64,
	PARAM_INT64,
	PARAM_CHAR_ARRAY,
	PARAM_WCHAR_ARRAY,
	PARAM_BLOB,
};

struct param
{
	enum param_type type;
	const char *name;
};

struct param_list
{
	struct param;
	struct param_list *next;
};

struct method
{
	struct param_list *request_params;
	struct param_list *response_params;
	const char *name;
};

struct method_list
{
	struct method *method;
	struct method_list *next;
};

struct interface
{
	struct method_list *methods;
	const char *name;
};

void generate_server_interface_c(struct interface *interface, FILE *out_file)
{
	struct method_list *method_list;

	fprintf(out_file, "/* auto-generated c code for the [%s] interface */\n", interface->name);

	method_list = interface->methods;
	while (method_list != NULL)
	{
		generate_server_method_c(interface->name, method_list->method, out_file);
		method_list = method_list->next;
	}

	fprintf(out_file, "static const mrpc_method_server_description *all_method_descriptions[] =\n{\n");
	method_list = interface->methods;
	while (method_list != NULL)
	{
		struct method *method;

		method = method_list->method;
		fprintf(out_file, "\t&%s_method_description,\n", method->name);
		method_list = method_list->next;
	}
	fprintf(out_file, "\tNULL\n};\n");

	fprintf(out_file, "/* the %s interface constructor. Use it in order to create %s interface */\n", interface->name, interface->name);
	fprintf(out_file, "struct mrpc_interface *%s_interface_create()\n"
		"{\n"
		"\tstruct mrpc_interface *interface;\n"
		"\n"
		"\tinterface = mrpc_interface_server_create(all_method_descriptions);\n"
		"\treturn interface;\n"
		"}\n",
		interface->name
	);
}

void generate_server_method_c(const char *interface_name, struct method *method, FILE *out_file)
{
	fprintf(out_file, "/* start of the [%s] method */\n", method->name);
	generate_params_c(method->name, "request", method->request_params, out_file);
	generate_params_c(method->name, "response", method->response_params, out_file);
	generate_server_callback_c(interface_name, method, out_file);
	fprintf(out_file,
		"static const struct mrpc_method_server_description %s_method_description =\n"
		"{\n"
		"\t%s_request_param_constructors,\n"
		"\t%s_response_param_constructors,\n"
		"\t%s_callback\n"
		"};\n",
		method->name, method->name, method->name, method->name
	);
	fprintf(out_file, "/* end of the [%s] method */\n\n", method->name);
}

void generate_params_c(const char *method_name, const char *type, struct param_list *param_list, FILE *out_file)
{
	fprintf(out_file, "static const mrpc_param_constructor %s_%s_param_constructors[] =\n{\n", method_name, type);
	while (param_list != NULL)
	{
		struct param *param;

		param = param_list->param;
		fprintf(out_file, "\t%s, /* %s */\n", param_type_to_constructor(param->type), param->name);
		param_list = param_list->next;
	}
	fprintf(out_file, "\tNULL\n};\n");
}

void generate_server_callback_c(const char *interface_name, struct method *method, FILE *out_file)
{
	struct param_list *param_list;
	struct param *param;
	int i;

	fprintf(out_file,
		"static void %s_callback(struct mrpc_data *data, void *service_ctx)\n"
		"{\n",
		method->name);

	generate_callback_params_c(method->request_params, "request", out_file);
	generate_callback_params_c(method->response_params, "response", out_file);

	fprintf(out_file, "\t/* obtain request parameters */\n");
	param_list = method->request_params;
	i = 0;
	while (param_list != NULL)
	{
		param = param_list->param;
		fprintf(out_file, "\tmrpc_data_get_request_param_value(data, %d, &request_%s);\n", i, param->name);
		param_list = param_list->next;
		i++;
	}
	fprintf(out_file, "\n");

	fprintf(out_file, "\t/* invoke service function */\n");
	fprintf(out_file, "\tservice_%s_%s(\n", interface_name, method->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		fprintf(out_file, "\t\trequest_%s,\n", param->name);
	}
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		fprintf(out_file, "\t\t&response_%s,\n", param->name);
	}
	fprintf(out_file, "\t\tservice_ctx);\n\n");

	fprintf(out_file, "\t/* set response paramters */\n");
	param_list = method->response_params;
	i = 0;
	while (param_list != NULL)
	{
		param = param_list->param;
		fprintf(out_file, "\tmrpc_data_set_response_param_value(data, %d, response_%s);\n", i, param->name);
		param_list = param_list->next;
		i++;
	}

	fprintf(out_file, "}\n");
}

void generate_callback_params_c(struct param_list *param_list, const char *type, FILE *out_file)
{
	while (param_list != NULL)
	{
		struct param *param;

		param = param_list->param;
		fprintf(out_file, "\t%s *%s_%s;\n", param_type_to_c_type(param->type), type, param->name);
		param_list = param_list->next;
	}
	fprintf(out_file, "\n");
}

const char *param_type_to_c_type(enum param_type param_type)
{
	switch (param_type)
	{
		case PARAM_UINT32: return "uint32_t";
		case PARAM_INT32: return "int32_t";
		case PARAM_UINT64: return "uint64_t";
		case PARAM_INT64: return "int64_t";
		case PARAM_CHAR_ARRAY: return "struct mrpc_char_array";
		case PARAM_WCHAR_ARRAY: return "struct mrpc_wchar_array";
		case PARAM_BLOB: return "struct mrpc_blob";
		default: return "unknown_type";
	}
}

const char *param_type_to_constructor(enum param_type param_type)
{
	switch (param_type)
	{
		case PARAM_UINT32: return "mrpc_uint32_param_create";
		case PARAM_INT32: return "mrpc_int32_param_create";
		case PARAM_UINT64: return "mrpc_uint64_param_create";
		case PARAM_INT64: return "mrpc_int64_param_create";
		case PARAM_CHAR_ARRAY: return "mrpc_char_array_param_create";
		case PARAM_WCHAR_ARRAY: return "mrpc_wchar_array_param_create";
		case PARAM_BLOB: return "mrpc_blob_param_create";
		default: return "unknown_constructor";
	}
}

-------------------------------------------------------------------------------
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
