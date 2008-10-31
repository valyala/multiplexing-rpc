#include "private/mrpc_common.h"

#include "private/mrpc_interface.h"
#include "private/mrpc_method_factory.h"
#include "private/mrpc_method.h"

/**
 * the maximum number of methods in the interface.
 * This number is limited by 0x100, because method_id field is encoded in one byte in the rpc client-server protocol.
 */
#define MAX_METHODS_CNT 0x100

struct mrpc_interface
{
	struct mrpc_method **methods;
	int methods_cnt;
};

static int get_constructors_cnt(const mrpc_method_constructor *method_constructors)
{
	int constructors_cnt;
	mrpc_method_constructor method_constructor;

	constructors_cnt = -1;
	do
	{
		constructors_cnt++;
		method_constructor = method_constructors[constructors_cnt];
	} while (constructors_cnt <= MAX_METHODS_CNT && method_constructor != NULL);
	ff_assert(constructors_cnt <= MAX_METHODS_CNT);

	return constructors_cnt;
}

struct mrpc_interface *mrpc_interface_create(const mrpc_method_constructor *method_constructors)
{
	struct mrpc_interface *interface;
	int i;

	interface = (struct mrpc_interface *) ff_malloc(sizeof(*interface));
	interface->methods_cnt = get_constructors_cnt(method_constructors);
	interface->methods = (struct mrpc_method **) ff_calloc(interface->methods_cnt, sizeof(interface->methods[0]));
	for (i = 0; i < interface->methods_cnt; i++)
	{
		mrpc_method_constructor method_constructor;

		method_constructor = method_constructors[i];
		interface->methods[i] = method_constructor();
	}

	return interface;
}

void mrpc_interface_delete(struct mrpc_interface *interface)
{
	int i;

	for (i = 0; i < interface->methods_cnt; i++)
	{
		struct mrpc_method *method;

		method = interface->methods[i];
		mrpc_method_delete(method);
	}

	ff_free(interface->methods);
	ff_free(interface);
}

struct mrpc_method *mrpc_interface_get_method(struct mrpc_interface *interface, uint8_t method_id)
{
	struct mrpc_method *method = NULL;

	ff_assert(method_id < MAX_METHODS_CNT);

	if (method_id >= 0 && method_id < interface->methods_cnt)
	{
		method = interface->methods[method_id];
	}

	return method;
}
