#include "private/mrpc_common.h"

#include "private/mrpc_interface.h"
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

static int get_methods_cnt(const void **method_descriptions)
{
	int methods_cnt;
	const void *method_description;

	methods_cnt = -1;
	do
	{
		methods_cnt++;
		method_description = method_descriptions[methods_cnt];
	} while (methods_cnt <= MAX_METHODS_CNT && method_description != NULL);
	ff_assert(methods_cnt <= MAX_METHODS_CNT);

	return methods_cnt;
}

struct mrpc_interface *mrpc_interface_client_create(const struct mrpc_method_client_description **method_descriptions)
{
	struct mrpc_interface *interface;
	int i;

	interface = (struct mrpc_interface *) ff_malloc(sizeof(*interface));
	interface->methods_cnt = get_methods_cnt((const void **) method_descriptions);
	ff_assert(interface->methods_cnt > 0);
	ff_assert(interface->methods_cnt <= MAX_METHODS_CNT);
	interface->methods = (struct mrpc_method **) ff_calloc(interface->methods_cnt, sizeof(interface->methods[0]));
	for (i = 0; i < interface->methods_cnt; i++)
	{
		const struct mrpc_method_client_description *method_description;
		struct mrpc_method *method;

		method_description = method_descriptions[i];
		ff_assert(method_description != NULL);
		method = mrpc_method_create_client_method(method_description->request_param_constructors, method_description->response_param_constructors, method_description->is_key, (uint8_t) i);
		ff_assert(method != NULL);
		interface->methods[i] = method;
	}

	return interface;
}

struct mrpc_interface *mrpc_interface_server_create(const struct mrpc_method_server_description **method_descriptions)
{
	struct mrpc_interface *interface;
	int i;

	interface = (struct mrpc_interface *) ff_malloc(sizeof(*interface));
	interface->methods_cnt = get_methods_cnt((const void **) method_descriptions);
	ff_assert(interface->methods_cnt > 0);
	ff_assert(interface->methods_cnt <= MAX_METHODS_CNT);
	interface->methods = (struct mrpc_method **) ff_calloc(interface->methods_cnt, sizeof(interface->methods[0]));
	for (i = 0; i < interface->methods_cnt; i++)
	{
		const struct mrpc_method_server_description *method_description;
		struct mrpc_method *method;

		method_description = method_descriptions[i];
		ff_assert(method_description != NULL);
		method = mrpc_method_create_server_method(method_description->request_param_constructors, method_description->response_param_constructors, method_description->callback, (uint8_t) i);
		ff_assert(method != NULL);
		interface->methods[i] = method;
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
		ff_assert(method != NULL);
		mrpc_method_delete(method);
	}

	ff_free(interface->methods);
	ff_free(interface);
}

struct mrpc_method *mrpc_interface_get_method(struct mrpc_interface *interface, uint8_t method_id)
{
	struct mrpc_method *method = NULL;

	/* there is no need to check if method_id >= 0 and method_id <= MAX_METHODS_CNT,
	 * because uint8_t range is limited between 0 and MAX_METHODS_CNT (256)
	 */
	if (method_id < interface->methods_cnt)
	{
		method = interface->methods[method_id];
	}
	else
	{
		ff_log_debug(L"the interface=%p doesn't contain method with method_id=%lu. It contains only %d methods", interface, (uint32_t) method_id, interface->methods_cnt);
	}

	return method;
}
