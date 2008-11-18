#include "common.h"
#include "c_server_interface_generator.h"
#include "c_client_interface_generator.h"

#include "types.h"

static struct param *create_param(int id, const char *type)
{
	struct param *param;
	char *name;

	name = (char *) calloc(20, sizeof(name[0]));
	sprintf(name, "param_%s_%d", type, id);

	param = (struct param *) malloc(sizeof(*param));
	param->name = name;
	param->type = (enum param_type) (id % 7);
	param->is_key = (id & 1);
	return param;
}

static struct param_list *create_params(int id, const char *type)
{
	struct param_list *param_list = NULL;
	int i;

	for (i = 0; i < id; i++)
	{
		struct param_list *tmp;

		tmp = (struct param_list *) malloc(sizeof(*tmp));
		tmp->param = create_param(i, type);
		tmp->next = param_list;
		param_list = tmp;
	}
	return param_list;
}

static struct method *create_method(int id)
{
	struct method *method;
	char *name;

	name = (char *) calloc(20, sizeof(name[0]));
	sprintf(name, "method_%d", id);

	method = (struct method *) malloc(sizeof(*method));
	method->name = name;
	method->request_params = create_params(id, "request");
	method->response_params = create_params(id, "response");
	return method;
}

static struct method_list *create_methods()
{
	struct method_list *method_list = NULL;
	int i;

	for (i = 0; i < 8; i++)
	{
		struct method_list *tmp;

		tmp = (struct method_list *) malloc(sizeof(*tmp));
		tmp->method = create_method(i);
		tmp->next = method_list;
		method_list = tmp;
	}
	return method_list;
}

static struct interface *create_interface()
{
	struct interface *interface;

	interface = (struct interface *) malloc(sizeof(*interface));
	interface->name = "caching";
	interface->methods = create_methods();
	return interface;
}

int main()
{
	struct interface *interface;

	interface = create_interface();
	c_server_interface_generator_generate(interface);
	c_client_interface_generator_generate(interface);
	return 0;
}
