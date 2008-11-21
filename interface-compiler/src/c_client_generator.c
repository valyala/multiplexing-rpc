#include "common.h"
#include "c_client_generator.h"

#include "c_common.h"
#include "types.h"

static void dump_client_method_declaration(const struct interface *interface, const struct method *method)
{
	const struct param_list *param_list;
	const struct param *param;

	dump("/* invokes the rpc method [%s] of the client interface [%s] using the given client.\n", method->name, interface->name);
	dump(" * The caller must be responsible for deleting returned response parameters.\n"
		 " * Returns FF_SUCCESS on success, FF_FAILURE on error.\n"
		 " * If the function returns FF_FAILURE, then there is no need to delete response paramters,\n"
		 " * because they aren't set in this case.\n"
		 " */\n"
	);
	dump("enum ff_result client_%s_%s(struct mrpc_client *client", interface->name, method->name);
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

static void dump_client_method(const struct interface *interface, const struct method *method, int id)
{
	const struct param_list *param_list;
	const struct param *param;

	dump_client_method_declaration(interface, method);
	dump("\n{\n");

	dump("\tstruct ff_stream *stream;\n");
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		dump("\t%sresponse_%s%s;\n", c_get_param_code_type(param), param->name, (c_is_param_ptr(param) ? " = NULL" : ""));
		param_list = param_list->next;
	}
	dump("\tuint8_t method_id = %d;\n", id);
	dump("\tenum ff_result result;\n\n");

	dump("\tstream = mrpc_client_create_request_stream(client);\n"
		 "\tif (stream == NULL)\n\t{\n"
		 "\t\tff_log_debug(L\"cannot create request stream using the client=%%p and tries_cnt=%%d. See previous messages for more info\", client, REQUEST_STREAM_TRIES_CNT);\n"
		 "\t\tgoto end;\n\t}\n\n"
	);

	dump("\tresult = ff_stream_write(stream, &method_id, 1);\n"
	     "\tif (result != FF_SUCCESS)\n\t{\n"
		 "\t\tff_log_debug(L\"cannot write method_id=%%d to the request stream=%%p. See previous messages for more info\", method_id, stream);\n"
		 "\t\tgoto end;\n\t}\n\n"
	);

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
			dump("\tresult = mrpc_%s_serialize(%s, stream);\n", c_get_param_type(param), param->name);
			dump("\tif (result != FF_SUCCESS)\n\t{\n");
			dump("\t\tff_log_debug(L\"cannot serialize the %s of the type %s to the stream=%%p. See previous messages for more info\", stream);\n", param->name, c_get_param_type(param));
			dump("\t\tgoto end;\n\t}\n");
			param_list = param_list->next;
		}
		while (param_list != NULL);
	}

	dump("\n\tresult = ff_stream_flush(stream);\n");
	dump("\tif (result != FF_SUCCESS)\n\t{\n"
		 "\t\tff_log_debug(L\"cannot flush the stream=%%p. See previous messages for more info\", stream);\n"
		 "\t\tgoto end;\n\t}\n\n");

	param_list = method->response_params;
	if (param_list == NULL)
	{
		dump("\t/* there is no response parameters */\n"
			 "\t{\n\t\tuint8_t empty_byte;\n\n"
			 "\t\tresult = ff_stream_read(stream, &empty_byte, 1);\n"
			 "\t\tif (result != FF_SUCCESS)\n\t\t{\n"
			 "\t\t\tff_log_debug(L\"cannot read an empty byte from the stream=%%p. See previous messages for more info\", stream);\n"
			 "\t\t\tgoto end;\n\t\t}\n"
			 "\t\tif (empty_byte != 0)\n\t\t{\n"
			 "\t\t\tff_log_debug(L\"unexpected empty_byte=%%d read from the stream. Expected 0\", (int) empty_byte);\n"
			 "\t\t\tgoto end;\n\t\t}\n\t}\n"
		);
	}
	else
	{
		do
		{
			param = param_list->param;
			dump("\tresult = mrpc_%s_unserialize(&response_%s, stream);\n", c_get_param_type(param), param->name);
			dump("\tif (result != FF_SUCCESS)\n\t{\n");
			dump("\t\tff_log_debug(L\"cannot unserialize the %s of the type %s from the stream=%%p. See previous messages for more info\", stream);\n", param->name, c_get_param_type(param));
			dump("\t\tgoto end;\n\t}\n");
			param_list = param_list->next;
		}
		while (param_list != NULL);

		dump("\n");
		param_list = method->response_params;
		while (param_list != NULL)
		{
			param = param_list->param;
			dump("\t*%s = response_%s;\n", param->name, param->name);
			if (c_is_param_ptr(param))
			{
				dump("\tmrpc_%s_inc_ref(response_%s);\n", c_get_param_type(param), param->name);
			}
			param_list = param_list->next;
		}
	}

	dump("\nend:\n");
	param_list = method->response_params;
	while (param_list != NULL)
	{
		param = param_list->param;
		if (c_is_param_ptr(param))
		{
			dump("\tif (response_%s != NULL)\n\t{\n\t\tmrpc_%s_dec_ref(response_%s);\n\t}\n", param->name, c_get_param_type(param), param->name);
		}
		param_list = param_list->next;
	}
	dump("\tif (stream != NULL)\n\t{\n\t\tff_stream_delete(stream);\n\t}\n");

	dump("\treturn result;\n}\n");
}

static void dump_client_source(const struct interface *interface)
{
	const struct method_list *method_list;
	const struct method *method;
	int i;

	dump("/* auto-generated code of the client for the interface [%s] */\n", interface->name);

	dump("#include \"mrpc/mrpc_common.h\"\n\n");
	dump("#include \"client_%s.h\"\n\n", interface->name);
	dump("#include \"mrpc/mrpc_int.h\"\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
	);
	dump("#include \"mrpc/mrpc_client.h\"\n"
		 "#include \"ff/ff_stream.h\"\n"
	);

    method_list = interface->methods;
	i = 0;
    while (method_list != NULL)
    {
    	method = method_list->method;
		dump("\n");
    	dump_client_method(interface, method, i);
    	method_list = method_list->next;
    	i++;
    }
}

static void dump_client_header(const struct interface *interface)
{
	const struct method_list *method_list;
	const struct method *method;

	dump("#ifndef CLIENT_%s_H\n#define CLIENT_%s_H\n\n", interface->name, interface->name);
	dump("#include \"mrpc/mrpc_common.h\"\n\n"
		 "#include \"mrpc/mrpc_int.h\"\n"
		 "#include \"mrpc/mrpc_blob.h\"\n"
		 "#include \"mrpc/mrpc_char_array.h\"\n"
		 "#include \"mrpc/mrpc_wchar_array.h\"\n\n"
		 "#include \"mrpc/mrpc_client.h\"\n\n"
	);
	dump("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

	method_list = interface->methods;
	while (method_list != NULL)
	{
		method = method_list->method;
		dump_client_method_declaration(interface, method);
		dump(";\n\n");
		method_list = method_list->next;
	}

	dump("#ifdef __cplusplus\n}\n#endif\n\n");
	dump("#endif\n");
}

void c_client_generator_generate(const struct interface *interface)
{
	const char *interface_name;

	interface_name = interface->name;

	open_file("client_%s.c", interface_name);
	dump_client_source(interface);
	close_file();
	printf("client_%s.c file has been generated\n", interface_name);

	open_file("client_%s.h", interface_name);
	dump_client_header(interface);
	close_file();
	printf("client_%s.h file has been generated\n", interface_name);
}
