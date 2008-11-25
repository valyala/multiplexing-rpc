#include "private/mrpc_common.h"
#include "private/mrpc_distributed_client_controller.h"
#include "ff/ff_stream_connector.h"

struct mrpc_distributed_client_controller
{
	const struct mrpc_distributed_client_controller_vtable *vtable;
	void *ctx;
};

struct mrpc_distributed_client_controller *mrpc_distributed_client_controller_create(const struct mrpc_distributed_client_controller_vtable *vtable, void *ctx)
{
	struct mrpc_distributed_client_controller *controller;

	controller = (struct mrpc_distributed_client_controller *) ff_malloc(sizeof(*controller));
	controller->vtable = vtable;
	controller->ctx = ctx;

	return controller;
}

void mrpc_distributed_client_controller_delete(struct mrpc_distributed_client_controller *controller)
{
	ff_assert(controller != NULL);

	controller->vtable->delete(controller->ctx);
	ff_free(controller);
}

void mrpc_distributed_client_controller_initialize(struct mrpc_distributed_client_controller *controller)
{
	ff_assert(controller != NULL);

	controller->vtable->initialize(controller->ctx);
}

void mrpc_distributed_client_controller_shutdown(struct mrpc_distributed_client_controller *controller)
{
	ff_assert(controller != NULL);

	controller->vtable->shutdown(controller->ctx);
}

enum mrpc_distributed_client_controller_message_type mrpc_distributed_client_controller_get_next_message(struct mrpc_distributed_client_controller *controller,
	struct ff_stream_connector **stream_connector, uint64_t *key)
{
	enum mrpc_distributed_client_controller_message_type message_type;

	ff_assert(controller != NULL);
	ff_assert(stream_connector != NULL);
	ff_assert(key != NULL);

	message_type = controller->vtable->get_next_message(controller->ctx, stream_connector, key);

	return message_type;
}
