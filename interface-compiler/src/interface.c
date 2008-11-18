#include "common.h"

#include "interface.h"

#include "types.h"

static void delete_param(struct param *param)
{
	free((void *) param->name);
	free(param);
}

static void delete_params(struct param_list *param_list)
{
	while (param_list != NULL)
	{
		struct param_list *prev_entry;

		delete_param(param_list->param);
		prev_entry = param_list;
		param_list = param_list->next;
		free(prev_entry);
	}
}

static void delete_method(struct method *method)
{
	delete_params(method->request_params);
	delete_params(method->response_params);
	free((void *) method->name);
	free(method);
}

static void delete_methods(struct method_list *method_list)
{
	while (method_list != NULL)
	{
		struct method_list *prev_entry;

		delete_method(method_list->method);
		prev_entry = method_list;
		method_list = method_list->next;
		free(prev_entry);
	}
}

void interface_delete(struct interface *interface)
{
	delete_methods(interface->methods);
	free((void *) interface->name);
}
