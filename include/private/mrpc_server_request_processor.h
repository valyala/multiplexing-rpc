#ifndef MRPC_SERVER_REQUEST_PROCESSOR_PRIVATE_H
#define MRPC_SERVER_REQUEST_PROCESSOR_PRIVATE_H

#include "private/mrpc_common.h"
#include "private/mrpc_packet.h"
#include "private/mrpc_packet_stream.h"
#include "private/mrpc_interface.h"
#include "ff/ff_blocking_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_server_request_processor;

/**
 * This callback must release the given request_processor with the given request_id
 */
typedef void (*mrpc_server_request_processor_release_func)(void *release_func_ctx, struct mrpc_server_request_processor *request_processor, uint8_t request_id);

/**
 * This callback must register error notification, which can be sent by request processor.
 */
typedef void (*mrpc_server_request_processor_notify_error_func)(void *notify_error_func_ctx);

/**
 * Creates the request processor, which is used for processing server rpc requests.
 * release_func is called when the request initiated by the mrpc_server_request_processor_start(), was finished.
 * notify_error_func is called on error occured while processing current request.
 * acquire_packet_func and release_packet_func are used for acquiring packets required for serializing rpc response
 * and for releasing packets received from the mrpc_server_request_processor_push_packet()
 * after they were parsed into request.
 * writer_queue is used for pushing serialized response packets. Packets from the writer_queue must
 * be released using the same technique as used by the release_packet_func() callback.
 * The max_reader_queue_size is the maximum number of packets, which can be accumulated in the reader queue,
 * when calling the mrpc_server_request_processor_push_packet() function.
 * Always returns correct result.
 */
struct mrpc_server_request_processor *mrpc_server_request_processor_create(mrpc_server_request_processor_release_func release_func, void *release_func_ctx,
	mrpc_server_request_processor_notify_error_func notify_error_func, void *notify_error_func_ctx,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx,
	struct ff_blocking_queue *writer_queue, int max_reader_queue_size);

/**
 * Deletes the given request_processor.
 */
void mrpc_server_request_processor_delete(struct mrpc_server_request_processor *request_processor);

/**
 * Starts processing request with the given request_id using the given request_processor.
 * service_interface and service_ctx are used for invoking corresponding server callback.
 */
void mrpc_server_request_processor_start(struct mrpc_server_request_processor *request_processor, const struct mrpc_interface *service_interface, void *service_ctx, uint8_t request_id);

/**
 * Notifies the given request_processor that it must stop processing the current request ASAP.
 * request_processor will invoke the release_func() callback when it will finish processing the current request.
 * Usually this function is used for graceful shutdown of the request_processor.
 */
void mrpc_server_request_processor_stop_async(struct mrpc_server_request_processor *request_processor);

/**
 * Pushes the given packet to the reader queue of the request_processor.
 * The packet must be acquired using the same technique as used by the acquire_packet_func() callback.
 */
void mrpc_server_request_processor_push_packet(struct mrpc_server_request_processor *request_processor, struct mrpc_packet *packet);

#ifdef __cplusplus
}
#endif

#endif
