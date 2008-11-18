#include "common.h"
#include "c_client_interface_generator.h"

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

static void dump_method(struct interface *interface, struct method *method)
{
	struct param_list *param_list;
	struct param *param;

	dump("/* start of the client method [%s] */\n", method->name);

	dump("static const mrpc_param_constructor client_request_param_constructors_%s_%s[] = \n{\n", interface->name, method->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%s, /* %s */\n", get_param_code_constructor(param), param->name);
		param_list = param_list->next;
	}
	dump("\tNULL\n};\n");

	dump("static const mrpc_param_constructor client_response_param_constructors_%s_%s[] = \n{\n", interface->name, method->name);
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%s, /* %s */\n", get_param_code_constructor(param), param->name);
		param_list = param_list->next;
	}
	dump("\tNULL\n};\n");

	dump("static const int client_is_key_%s_%s[] = \n{\n", interface->name, method->name);
	param_list = method->request_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%d, /* %s */\n", param->is_key, param->name);
		param_list = param_list->next;
	}
	dump("\t0\n};\n");

	dump("static const struct mrpc_method_client_description client_method_description_%s_%s =\n{\n", interface->name, method->name);
	dump("\tclient_request_param_constructors_%s_%s,\n", interface->name, method->name);
	dump("\tclient_response_param_constructors_%s_%s,\n", interface->name, method->name);
	dump("\tclient_is_key_%s_%s\n};\n", interface->name, method->name);

	dump("/* end of the client method [%s] */\n\n", method->name);
}

static void dump_interface_constructor_declaration(struct interface *interface)
{
	dump("/* creates the client interface [%s].\n", interface->name);
	dump(" * delete the interface instance using the mrpc_interface_delete() method\n */\n");
	dump("struct mrpc_interface *client_interface_%s_create()", interface->name);
}

static void dump_interface_source(struct interface *interface)
{
	struct method_list *method_list;
	struct method *method;

	dump("/* auto-generated code for the client interface [%s] */\n", interface->name);

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"client_interface_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_interface.h\"\n"
	     "#include \"mrpc/mrpc_method.h\"\n"
	     "#include \"mrpc/mrpc_param_constructors.h\"\n\n"
	);

	dump("/* start of the client methods' declarations */\n\n");
	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump_method(interface, method);
		method_list = method_list->next;
	}
	dump("/* end of the client methods' declarations */\n\n");

	dump("static const struct mrpc_method_client_description *client_method_descriptions_%s[] =\n{\n", interface->name);
	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump("\t&client_method_description_%s_%s,\n", interface->name, method->name);
		method_list = method_list->next;
	}
	dump("\tNULL\n};\n\n");

	dump_interface_constructor_declaration(interface);
	dump("\n{\n\tstruct mrpc_interface *interface;\n\n"
		 "\tinterface = mrpc_interface_client_create(client_method_descriptions_%s);\n"
		 "\treturn interface;\n}\n",
		 interface->name
	);
}

static void dump_interface_header(struct interface *interface)
{
	dump("#ifndef CLIENT_INTERFACE_%s_H\n#define CLIENT_INTERFACE_%s_H\n\n", interface->name, interface->name);
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

	dump("/* implements the [%s] method of the client interface [%s].\n", method->name, interface->name);
	dump(" * The interface must be created using client_interface_%s_create() function.\n", interface->name);
	dump(" * The caller must be responsible for deleting returned response parameters.\n"
		 " * Returns FF_SUCCESS on success, FF_FAILURE on error.\n"
		 " * If the function returns FF_FAILURE, then there is no need to delete response paramters,\n"
		 " * because they aren't set in this case.\n"
		 " */\n"
	);
	dump("enum ff_result client_service_%s_%s(struct mrpc_interface *interface, struct mrpc_client *client", interface->name, method->name);
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

static void dump_service_method(struct interface *interface, struct method *method, int id)
{
	struct param_list *param_list;
	struct param *param;
	int i;

	dump_service_method_declaration(interface, method);
	dump("\n{\n");

	dump("\tstruct mrpc_data *data;\n");
	dump("\tenum ff_result result;\n\n");
	dump("\tdata = mrpc_data_create(interface, %d);\n", id);
	dump("\tff_assert(data != NULL);\n\n");

	param_list = method->request_params;
	i = 0;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\tmrpc_data_set_request_param_value(data, %d, request_%s);\n", i, param->name);
		param_list = param_list->next;
		i++;
	}

	dump("\tresult = mrpc_client_invoke_rpc(client, data);\n");
	dump("\tif (result != FF_SUCCESS)\n\t{\n");
	dump("\t\tff_log_debug(\"cannot invoke rpc with id=%d for the interface=%%p, client=%%p. See previous messages for more info\", interface, client);\n", id);
	dump("\t\tgoto end;\n\t}\n");

	i = 0;
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\tmrpc_data_get_response_param_value(data, %d, response_%s);\n", i, param->name);
		param_list = param_list->next;
		i++;
	}
	dump("\n");

	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		switch (param->type)
		{
		case PARAM_UINT32:
			dump("\t{\n\t\tuint32_t *tmp;\n\n");
			dump("\t\ttmp = (uint32_t *) ff_malloc(sizeof(*tmp));\n");
			dump("\t\t*tmp = **response_%s;\n", param->name);
			dump("\t\t*response = tmp;\n\t}\n");
			break;
		case PARAM_INT32:
			dump("\t{\n\t\tint32_t *tmp;\n\n");
			dump("\t\ttmp = (int32_t *) ff_malloc(sizeof(*tmp));\n");
			dump("\t\t*tmp = **response_%s;\n", param->name);
			dump("\t\t*response = tmp;\n\t}\n");
			break;
		case PARAM_UINT64:
			dump("\t{\n\t\tuint64_t *tmp;\n\n");
			dump("\t\ttmp = (uint64_t *) ff_malloc(sizeof(*tmp));\n");
			dump("\t\t*tmp = **response_%s;\n", param->name);
			dump("\t\t*response = tmp;\n\t}\n");
			break;
		case PARAM_INT64:
			dump("\t{\n\t\tint64_t *tmp;\n\n");
			dump("\t\ttmp = (int64_t *) ff_malloc(sizeof(*tmp));\n");
			dump("\t\t*tmp = **response_%s;\n", param->name);
			dump("\t\t*response = tmp;\n\t}\n");
			break;
		case PARAM_CHAR_ARRAY:
			dump("\tmrpc_char_array_inc_ref(*response_%s);\n", param->name);
			break;
		case PARAM_WCHAR_ARRAY:
			dump("\tmrpc_wchar_array_inc_ref(*response_%s);\n", param->name);
			break;
		case PARAM_BLOB:
			dump("\tmrpc_blob_inc_ref(*response_%s);\n", param->name);
			break;
		default:
			die("parameter %s has unknown type", param->name);
			break;
		}
		param_list = param_list->next;
	}

	dump("end:\n"
		 "\tmrpc_data_delete(data);\n"
		 "\treturn result;\n}\n\n"
	);
}

static void dump_service_source(struct interface *interface)
{
	struct method_list *method_list;
	struct method *method;
	int i;

	dump("/* auto-generated code for the client service [%s] */\n", interface->name);

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"client_service_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);
	dump("#include \"mrpc/mrpc_interface.h\"\n"
		 "#include \"mrpc/mrpc_data.h\"\n"
		 "#include \"mrpc/mrpc_client.h\"\n\n"
	);

    method_list = interface->methods;
	i = 0;
    while (method_list != NULL)
    {
    	method = method_list->method;
    	dump_service_method(interface, method, i);
    	method_list = method_list->next;
    	i++;
    }
}

static void dump_service_header(struct interface *interface)
{
	struct method_list *method_list;
	struct method *method;

	dump("#ifndef CLIENT_SERVICE_%s_H\n#define CLIENT_SERVICE_%s_H\n\n", interface->name, interface->name);
	dump("#include \"mrpc/mrpc_common.h\"\n\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
		 "#include \"mrpc/mrpc_interface.h\"\n"
		 "#include \"mrpc/mrpc_client.h\"\n\n"
	);
	dump("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

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

void c_client_interface_generator_generate(struct interface *interface)
{
	const char *interface_name;

	interface_name = interface->name;

	open_file("client_interface_%s.c", interface_name);
	dump_interface_source(interface);
	close_file();
	printf("client_interface_%s.c file has been generated\n", interface_name);

	open_file("client_interface_%s.h", interface_name);
	dump_interface_header(interface);
	close_file();
	printf("client_interface_%s.h file has been generated\n", interface_name);

	open_file("client_service_%s.c", interface_name);
	dump_service_source(interface);
	close_file();
	printf("client_interface_%s.c file has been generated\n", interface_name);

	open_file("client_service_%s.h", interface_name);
	dump_service_header(interface);
	close_file();
	printf("client_interface_%s.h file has been generated\n", interface_name);
}
