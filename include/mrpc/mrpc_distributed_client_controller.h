#ifndef MRPC_DISTRIBUTED_CLIENT_CONTROLLER_PUBLIC_H
#define MRPC_DISTRIBUTED_CLIENT_CONTROLLER_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_distributed_client_controller;

enum mrpc_distributed_client_controller_message_type
{
	/**
	 * This message must be returned for adding the client with given key and given stream_connector
	 * to the distributed_client container.
	 */
	MRPC_DISTRIBUTED_CLIENT_ADD_CLIENT,

	/**
	 * This message must be returned for removing the client with given key
	 * from the distributed_client container.
	 */
	MRPC_DISTRIBUTED_CLIENT_REMOVE_CLIENT,

	/**
	 * this message must be returned before clients' list rebuilding.
	 */
	MRPC_DISTRIBUTED_CLIENT_REMOVE_ALL_CLIENTS,

	/**
	 * this message must be returned immediately only if the controller has been shutdowned or not initialized yet.
	 */
	MRPC_DISTRIBUTED_CLIENT_STOP
};

struct mrpc_distributed_client_controller_vtable
{
	/**
	 * deletes the given controller.
	 */
	void (*delete)(void *ctx);

	/**
	 * initializes the given controller.
	 */
	void (*initialize)(void *ctx);

	/**
	 * shutdowns the given controller. Subsequent calls to the get_next_message() must return MRPC_DISTRIBUTED_CLIENT_STOP
	 */
	void (*shutdown)(void *ctx);

	/**
	 * Returns the next message and corresponding data.
	 * both stream_connector and key must be set for the MRPC_DISTRIBUTED_CLIENT_ADD_CLIENT message.
	 * key must be set for the MRPC_DISTRIBUTED_CLIENT_REMOVE_CLIENT message.
	 * niether stream_connector nor key must be set for
	 * the MRPC_DISTRIBUTED_CLIENT_REMOVE_ALL_CLIENTS and MRPC_DISTRIBUTED_CLIENT_STOP messages.
	 * The MRPC_DISTRIBUTED_CLIENT_STOP message must be returned immediately if the controller
	 * has been shutdowned or not initialized yet.
	 */
	enum mrpc_distributed_client_controller_message_type (*get_next_message)(void *ctx, struct ff_stream_connector **stream_connector, uint64_t *key);
};

/**
 * creates distributed client controller from the given vtable and ctx.
 * Always returns correct result.
 */
MRPC_API struct mrpc_distributed_client_controller *mrpc_distributed_client_controller_create(const struct mrpc_distributed_client_controller_vtable *vtable, void *ctx);

/**
 * deletes the given controller.
 */
MRPC_API void mrpc_distributed_client_controller_delete(struct mrpc_distributed_client_controller *controller);

/**
 * initializes the given controller.
 */
MRPC_API void mrpc_distributed_client_controller_initialize(struct mrpc_distributed_client_controller *controller);

/**
 * shutdowns the given controller.
 * subsequent calls to the mrpc_distributed_client_controller_get_next_message() will immediately return MRPC_DISTRIBUTED_CLIENT_STOP.
 */
MRPC_API void mrpc_distributed_client_controller_shutdown(struct mrpc_distributed_client_controller *controller);

/**
 * returns the next message
 */
MRPC_API enum mrpc_distributed_client_controller_message_type mrpc_distributed_client_controller_get_next_message(struct mrpc_distributed_client_controller *controller,
	struct ff_stream_connector **stream_connector, uint64_t *key);

#ifdef __cplusplus
}
#endif

#endif
