#include "common.h"
#include "c_server_generator.h"

#include "c_common.h"
#include "types.h"

static void dump_server_method_handler(const struct interface *interface, const struct method *method)
{
	const struct param_list *param_list;
	const struct param *param;

	dump("static enum ff_result server_method_handler_%s_%s(struct ff_stream *stream, struct service_%s *service)\n{\n",
		interface->name, method->name, interface->name);

	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%srequest_%s%s;\n", c_get_param_code_type(param), param->name, (c_is_param_ptr(param) ? " = NULL" : ""));
		param_list = param_list->next;
	}
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%sresponse_%s%s;\n", c_get_param_code_type(param), param->name, (c_is_param_ptr(param) ? " = NULL" : ""));
		param_list = param_list->next;
	}
	dump("\tenum ff_result result;\n\n");

	param_list = method->request_params;
	if (param_list == NULL)
	{
		dump("\t/* there is no request parameters */\n");
	}
	else
	{
		do
		{
			param = param_list->param;
			dump("\tresult = mrpc_%s_unserialize(&request_%s, stream);\n", c_get_param_type(param), param->name);
			dump("\tif (result != FF_SUCCESS)\n\t{\n");
			dump("\t\tff_log_debug(L\"cannot unserialize parameter %s of type %s from the stream=%%p. See previous messages for more info\", stream);\n",
				param->name, c_get_param_type(param));
			dump("\t\tgoto end;\n\t}\n");
			param_list = param_list->next;
		}
		while (param_list != NULL);
	}

	dump("\n\tservice_%s_%s(service", interface->name, method->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(", request_%s", param->name);
		param_list = param_list->next;
	}

	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(", &response_%s", param->name);
		param_list = param_list->next;
	}
	dump(");\n\n");

	param_list = method->response_params;
	if (param_list == NULL)
	{
		dump("\t/* there is no response paramters */\n"
			 "\t{\n\t\tuint8_t empty_byte = 0;\n\n"
			 "\t\tresult = ff_stream_write(stream, &empty_byte, 1);\n"
			 "\t\tif (result != FF_SUCCESS)\n\t\t{\n"
			 "\t\t\tff_log_debug(L\"cannot write an empty byte to the stream=%%p. See previous messages for more info\", stream);\n"
			 "\t\t\tgoto end;\n\t\t}\n\t}"
		);
	}
	else
	{
		do
		{
			param = param_list->param;
			dump("\tresult = mrpc_%s_serialize(response_%s, stream);\n", c_get_param_type(param), param->name);
			dump("\tif (result != FF_SUCCESS)\n\t{\n");
			dump("\t\tff_log_debug(L\"cannot serialize parameter %s of type %s to the stream=%%p. See previous messages for more info\", stream);\n",
				param->name, c_get_param_type(param));
			dump("\t\tgoto end;\n\t}\n");
			param_list = param_list->next;
		}
		while (param_list != NULL);
	}

	dump("\nend:\n");
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		if (c_is_param_ptr(param))
		{
			dump("\tif (request_%s != NULL)\n\t{\n\t\tmrpc_%s_dec_ref(request_%s);\n\t}\n",
				param->name, c_get_param_type(param), param->name);
		}
		param_list = param_list->next;
	}
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		if (c_is_param_ptr(param))
		{
			dump("\tif (response_%s != NULL)\n\t{\n\t\tmrpc_%s_dec_ref(response_%s);\n\t}\n",
				param->name, c_get_param_type(param), param->name);
		}
		param_list = param_list->next;
	}

	dump("\treturn result;\n}\n\n");
}

static void dump_server_stream_handler_declaration(const struct interface *interface)
{
	dump("/* mrpc_server_stream_handler for the interface [%s] */\n", interface->name);
	dump("enum ff_result server_stream_handler_%s(struct ff_stream *stream, void *service_ctx)",
		interface->name);
}

static void dump_server_stream_handler(const struct interface *interface, int methods_cnt)
{
	dump_server_stream_handler_declaration(interface);
	dump("\n{\n\tstruct service_%s *service;\n", interface->name);
	dump("\tuint8_t method_id;\n"
		 "\tenum ff_result result;\n\n"
	);
	dump("\tservice = (struct service_%s *) service_ctx;\n", interface->name);
	dump("\tresult = ff_stream_read(stream, &method_id, 1);\n"
		 "\tif (result != FF_SUCCESS)\n\t{\n"
		 "\t\tff_log_debug(L\"cannot read method_id from the stream=%%p. See previous messages for more info\", stream);\n"
		 "\t\tgoto end;\n\t}\n"
	);
	dump("\tif (method_id >= %d)\n\t{\n", methods_cnt);
	dump("\t\tff_log_debug(L\"unexpected method_id=%%d read from the stream=%%p. It must be less than %d\", (int) method_id, stream);\n", methods_cnt);
	dump("\t\tgoto end;\n\t}\n\n");
	dump("\tresult = server_method_handlers_%s[method_id](stream, service);\n", interface->name);
	dump("\tif (result != FF_SUCCESS)\n\t{\n"
		 "\t\tff_log_debug(L\"cannot handle the method with method_id=%%d using the stream=%%p. See previous messages for more info\", (int) method_id, stream);\n"
		 "\t\tgoto end;\n\t}\n\n"
	);
	dump("\tresult = ff_stream_flush(stream);\n"
		 "\tif (result != FF_SUCCESS)\n\t{\n"
		 "\t\tff_log_debug(L\"cannot flush the stream=%%p. See previous messages for more info\", stream);\n"
		 "\t\tgoto end;\n\t}\n\n"
	     "end:\n\treturn result;\n}\n"
	);
}

static void dump_server_stream_handler_source(const struct interface *interface)
{
	const struct method_list *method_list;
	const struct method *method;
	int methods_cnt;

	dump("/* auto-generated code of the mrpc_server_stream_handler for the interface [%s] */\n", interface->name);

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"server_stream_handler_%s.h\"\n", interface->name);
	dump("#include \"service_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_int.h\"\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n"
		 "#include \"mrpc/mrpc_server_stream_handler.h\"\n"
		 "#include \"ff/ff_stream.h\"\n\n"
	);
	dump("typedef enum ff_result (*server_method_handler)(struct ff_stream *stream, struct service_%s *service);\n\n", interface->name);

	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump_server_method_handler(interface, method);
		method_list = method_list->next;
	}

	dump("static const server_method_handler server_method_handlers_%s[] =\n{\n", interface->name);
	method_list = interface->methods;
	methods_cnt = 0;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump("\tserver_method_handler_%s_%s,\n", interface->name, method->name);
		method_list = method_list->next;
		methods_cnt++;
	}
	dump("};\n\n");

	dump_server_stream_handler(interface, methods_cnt);
}

static void dump_server_stream_handler_header(const struct interface *interface)
{
	dump("#ifndef SERVER_STREAM_HANDLER_%s_H\n#define SERVER_STREAM_HANDLER_%s_H\n\n", interface->name, interface->name);
	dump("#include \"mrpc/mrpc_common.h\"\n"
		 "#include \"mrpc/mrpc_server_stream_handler.h\"\n"
		 "#include \"ff/ff_stream.h\"\n\n");

	dump("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

	dump_server_stream_handler_declaration(interface);
	dump(";\n\n");

	dump("#ifdef __cplusplus\n}\n#endif\n\n");
	dump("#endif\n");
}

static void dump_service_method_declaration(const struct interface *interface, const struct method *method)
{
	const struct param_list *param_list;
	const struct param *param;

	dump("/* implements the server method [%s] of the interface [%s] */\n", method->name, interface->name);
	dump("void service_%s_%s(struct service_%s *service", interface->name, method->name, interface->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(", %s%s", c_get_param_code_type(param), param->name);
		param_list = param_list->next;
	}
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(", %s*%s", c_get_param_code_type(param), param->name);
		param_list = param_list->next;
	}
	dump(")");
}

static void dump_service_method(const struct interface *interface, const struct method *method)
{
	dump_service_method_declaration(interface, method);
	dump("\n{\n\t/* insert code for the [%s] method instead of ff_assert(0)*/\n", method->name);
	dump("\tff_assert(0);\n}\n");
}

static void dump_service_constructor_declaration(const struct interface *interface)
{
	dump("/* creates the service instance for the interface [%s] */\n", interface->name);
	dump("struct service_%s *service_%s_create()", interface->name, interface->name);
}

static void dump_service_destructor_declaration(const struct interface *interface)
{
	dump("/* deletes the service instance, which was created by service_%s_create() */\n", interface->name);
	dump("void service_%s_delete(struct service_%s *service)", interface->name, interface->name);
}

static void dump_service_source(const struct interface *interface)
{
	const struct method_list *method_list;
	const struct method *method;

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"service_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_int.h\"\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);

	dump("struct service_%s\n{\n\t/* put service context here */\n};\n\n", interface->name);

	dump_service_constructor_declaration(interface);
	dump("\n{\n\tstruct service_%s *service;\n\n", interface->name);
	dump("\tservice = (struct service_%s *) ff_malloc(sizeof(*service));\n\n", interface->name);

	dump("\t/* insert service initialization code instead of ff_assert(0) */\n"
		 "\tff_assert(0);\n\n"
		 "\treturn service;\n}\n\n"
	);

	dump_service_destructor_declaration(interface);
	dump("\n{\n\t/* insert service shutdown code instead of ff_assert(0) */\n"
		 "\tff_assert(0);\n\n"
		 "\tff_free(service);\n}\n"
    );

    method_list = interface->methods;
    while (method_list != NULL)
    {
    	method = method_list->method;
		dump("\n");
    	dump_service_method(interface, method);
    	method_list = method_list->next;
    }
}

static void dump_service_header(const struct interface *interface)
{
	const struct method_list *method_list;
	const struct method *method;

	dump("#ifndef SERVICE_%s_H\n#define SERVICE_%s_H\n\n", interface->name, interface->name);
	dump("#include \"mrpc/mrpc_common.h\"\n\n"
		 "#include \"mrpc/mrpc_int.h\"\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);
	dump("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

	dump("struct service_%s;\n\n", interface->name);

	dump_service_constructor_declaration(interface);
	dump(";\n\n");

	dump_service_destructor_declaration(interface);
	dump(";\n\n");

	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump_service_method_declaration(interface, method);
		dump(";\n\n");
		method_list = method_list->next;
	}

	dump("#ifdef __cplusplus\n}\n#endif\n\n");
	dump("#endif\n");
}

void c_server_generator_generate(const struct interface *interface)
{
	const char *interface_name;

	interface_name = interface->name;

	open_file("server_stream_handler_%s.c", interface_name);
	dump_server_stream_handler_source(interface);
	close_file();
	printf("server_stream_handler_%s.c file has been generated\n", interface_name);

	open_file("server_stream_handler_%s.h", interface_name);
	dump_server_stream_handler_header(interface);
	close_file();
	printf("server_stream_handler_%s.h file has been generated\n", interface_name);

	open_file("service_%s.c", interface_name);
	dump_service_source(interface);
	close_file();
	printf("service_%s.c file has been generated\n", interface_name);

	open_file("service_%s.h", interface_name);
	dump_service_header(interface);
	close_file();
	printf("service_%s.h file has been generated\n", interface_name);
}
