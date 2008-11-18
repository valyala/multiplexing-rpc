#include "common.h"
#include "c_server_interface_generator.h"

#include "types.h"

static const char *get_param_code_type(struct param *param)
{
	switch (param->type)
	{
		case PARAM_UINT32: return "uint32_t";
		case PARAM_INT32: return "int32_t";
		case PARAM_UINT64: return "uint64_t";
		case PARAM_INT64: return "int64_t";
		case PARAM_CHAR_ARRAY: return "struct mrpc_char_array";
		case PARAM_WCHAR_ARRAY: return "struct mrpc_wchar_array";
		case PARAM_BLOB: return "struct mrpc_blob";
		default: die("unknown_type for the parameter [%s]", param->name);
	}
	return NULL;
}

static const char *get_param_code_constructor(struct param *param)
{
	switch (param->type)
	{
		case PARAM_UINT32: return "mrpc_uint32_param_create";
		case PARAM_INT32: return "mrpc_int32_param_create";
		case PARAM_UINT64: return "mrpc_uint64_param_create";
		case PARAM_INT64: return "mrpc_int64_param_create";
		case PARAM_CHAR_ARRAY: return "mrpc_char_array_param_create";
		case PARAM_WCHAR_ARRAY: return "mrpc_wchar_array_param_create";
		case PARAM_BLOB: return "mrpc_blob_param_create";
		default: die("unknown_constructor for the parameter [%s]", param->name);
	}
	return NULL;
}

static void dump_callback(struct interface *interface, struct method *method)
{
	struct param_list *param_list;
	struct param *param;
	int i;

	dump("static void server_callback_%s_%s(struct mrpc_data *data, void *service_ctx)\n{\n", interface->name, method->name);

	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%s *request_%s;\n", get_param_code_type(param), param->name);
		param_list = param_list->next;
	}

	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%s *response_%s;\n", get_param_code_type(param), param->name);
		param_list = param_list->next;
	}

	dump("\tstruct server_service_%s *service;\n\n", interface->name);
	dump("\tservice = (struct server_service_%s *) service_ctx;\n\n", interface->name);

	i = 0;
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\tmrpc_data_get_request_param_value(data, %d, &request_%s);\n", i, param->name);
		param_list = param_list->next;
		i++;
	}

	dump("\tserver_service_%s_%s(service", interface->name, method->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(",\n\t\trequest_%s", param->name);
		param_list = param_list->next;
	}

	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(",\n\t\t&response_%s", param->name);
		param_list = param_list->next;
	}
	dump(");\n");

	param_list = method->response_params;
	i = 0;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\tmrpc_data_set_response_param_value(data, %d, response_%s);\n", i, param->name);
		param_list = param_list->next;
		i++;
	}

	dump("}\n");
}

static void dump_method(struct interface *interface, struct method *method)
{
	struct param_list *param_list;
	struct param *param;

	dump("/* start of the server method [%s] */\n", method->name);

	dump("static const mrpc_param_constructor server_request_param_constructors_%s_%s[] = \n{\n", interface->name, method->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%s, /* %s */\n", get_param_code_constructor(param), param->name);
		param_list = param_list->next;
	}
	dump("\tNULL\n};\n");

	dump("static const mrpc_param_constructor server_response_param_constructors_%s_%s[] = \n{\n", interface->name, method->name);
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%s, /* %s */\n", get_param_code_constructor(param), param->name);
		param_list = param_list->next;
	}
	dump("\tNULL\n};\n");

	dump_callback(interface, method);

	dump("static const struct mrpc_method_server_description server_method_description_%s_%s =\n{\n", interface->name, method->name);
	dump("\tserver_request_param_constructors_%s_%s,\n", interface->name, method->name);
	dump("\tserver_response_param_constructors_%s_%s,\n", interface->name, method->name);
	dump("\tserver_callback_%s_%s\n};\n", interface->name, method->name);

	dump("/* end of the server method [%s] */\n\n", method->name);
}

static void dump_interface_constructor_declaration(struct interface *interface)
{
	dump("/* creates the server interface [%s].\n", interface->name);
	dump(" * delete the interface instance using the mrpc_interface_delete() method\n */\n");
	dump("struct mrpc_interface *server_interface_%s_create()", interface->name);
}

static void dump_interface_source(struct interface *interface)
{
	struct method_list *method_list;
	struct method *method;

	dump("/* auto-generated code for the server interface [%s] */\n", interface->name);

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"server_interface_%s.h\"\n", interface->name);
	dump("#include \"server_service_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);
	dump("#include \"mrpc/mrpc_interface.h\"\n"
	     "#include \"mrpc/mrpc_method.h\"\n"
		 "#include \"mrpc/mrpc_data.h\"\n"
	     "#include \"mrpc/mrpc_param_constructors.h\"\n\n"
	);

	dump("/* start of server methods' declarations */\n\n");
	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump_method(interface, method);
		method_list = method_list->next;
	}
	dump("/* end of server methods' declarations */\n\n");

	dump("static const struct mrpc_method_server_description *server_method_descriptions_%s[] =\n{\n", interface->name);
	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump("\t&server_method_description_%s_%s,\n", interface->name, method->name);
		method_list = method_list->next;
	}
	dump("\tNULL\n};\n\n");

	dump_interface_constructor_declaration(interface);
	dump("\n{\n\tstruct mrpc_interface *interface;\n\n"
		 "\tinterface = mrpc_interface_server_create(server_method_descriptions_%s);\n"
		 "\treturn interface;\n}\n",
		 interface->name
	);
}

static void dump_interface_header(struct interface *interface)
{
	dump("#ifndef SERVER_INTERFACE_%s_H\n#define SERVER_INTERFACE_%s_H\n\n", interface->name, interface->name);
	dump("#include \"mrpc/mrpc_interface.h\"\n\n");
	dump("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

	dump_interface_constructor_declaration(interface);
	dump(";\n\n");

	dump("#ifdef __cplusplus\n}\n#endif\n\n");
	dump("#endif\n");
}

static void dump_service_method_declaration(struct interface *interface, struct method *method)
{
	struct param_list *param_list;
	struct param *param;

	dump("/* implements the server method [%s] of the interface [%s] */\n", method->name, interface->name);
	dump("void server_service_%s_%s(struct server_service_%s *service", interface->name, method->name, interface->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(",\n\t%s *request_%s", get_param_code_type(param), param->name);
		param_list = param_list->next;
	}
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump(",\n\t%s **response_%s", get_param_code_type(param), param->name);
		param_list = param_list->next;
	}
	dump(")");
}

static void dump_service_method(struct interface *interface, struct method *method)
{
	dump_service_method_declaration(interface, method);
	dump("\n{\n\t/* insert code for the [%s] method instead of ff_assert(0)*/\n", method->name);
	dump("\tff_assert(0);\n}\n\n");
}

static void dump_service_constructor_declaration(struct interface *interface)
{
	dump("/* creates the service instance for the server interface [%s] */\n", interface->name);
	dump("struct server_service_%s *server_service_%s_create()", interface->name, interface->name);
}

static void dump_service_destructor_declaration(struct interface *interface)
{
	dump("/* deletes the service instance, which was created by server_service_%s_create() */\n", interface->name);
	dump("void server_service_%s_delete(struct server_service_%s *service)", interface->name, interface->name);
}

static void dump_service_source(struct interface *interface)
{
	struct method_list *method_list;
	struct method *method;

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"server_service_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);

	dump("struct server_service_%s\n{\n"
		 "\t/* put service context here */\n};\n\n",
		 interface->name
	);

	dump_service_constructor_declaration(interface);
	dump("\n{\n\tstruct server_service_%s *service;\n\n"
		 "\tservice = (struct service *) ff_malloc(sizeof(*service));\n\n"
		 "\t/* insert service initialization code instead of ff_assert(0) */\n"
		 "\tff_assert(0);\n\n"
		 "\treturn service;\n}\n",
		 interface->name
	);
	dump("\n");

	dump_service_destructor_declaration(interface);
	dump("\n{\n\t/* insert service shutdown code instead of ff_assert(0) */\n"
		 "\tff_assert(0);\n\n"
		 "\tff_free(service);\n}\n"
    );
    dump("\n");

    method_list = interface->methods;
    while (method_list != NULL)
    {
    	method = method_list->method;
    	dump_service_method(interface, method);
    	method_list = method_list->next;
    }
}

static void dump_service_header(struct interface *interface)
{
	struct method_list *method_list;
	struct method *method;

	dump("#ifndef SERVER_SERVICE_%s_H\n#define SERVER_SERVICE_%s_H\n\n", interface->name, interface->name);
	dump("#include \"mrpc/mrpc_common.h\"\n\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);
	dump("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

	dump("struct server_service_%s;\n\n", interface->name);

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

void c_server_interface_generator_generate(struct interface *interface)
{
	const char *interface_name;

	interface_name = interface->name;

	open_file("server_interface_%s.c", interface_name);
	dump_interface_source(interface);
	close_file();
	printf("server_interface_%s.c file has been generated\n", interface_name);

	open_file("server_interface_%s.h", interface_name);
	dump_interface_header(interface);
	close_file();
	printf("server_interface_%s.h file has been generated\n", interface_name);

	open_file("server_service_%s.c", interface_name);
	dump_service_source(interface);
	close_file();
	printf("server_interface_%s.c file has been generated\n", interface_name);

	open_file("server_service_%s.h", interface_name);
	dump_service_header(interface);
	close_file();
	printf("server_interface_%s.h file has been generated\n", interface_name);
}
