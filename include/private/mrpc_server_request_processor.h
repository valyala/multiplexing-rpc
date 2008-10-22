#ifndef MRPC_SERVER_REQUEST_PROCESSOR_PRIVATE
#define MRPC_SERVER_REQUEST_PROCESSOR_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_packet.h"
#include "private/mrpc_packet_stream.h"
#include "private/mrpc_interface.h"
#include "ff/ff_blocking_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_server_request_processor;

typedef void (*mrpc_request_processor_release_func)(void *release_func_ctx, struct mrpc_server_request_processor *request_processor, uint8_t request_id);

typedef void (*mrpc_request_processor_notify_error_func)(void *notify_error_func_ctx);

struct mrpc_server_request_processor *mrpc_server_request_processor_create(mrpc_server_request_processor_release_func release_func, void *release_func_ctx,
	mrpc_server_request_processor_notify_error_func notify_error_func, void *notify_error_func_ctx,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx,
	struct mrpc_interface *service_interface, void *service_ctx, struct ff_blocking_queue *writer_queue);

void mrpc_server_request_processor_delete(struct mrpc_server_request_processor *request_processor);

void mrpc_server_request_processor_start(struct mrpc_server_request_processor *request_processor, uint8_t request_id);

void mrpc_server_request_processor_stop_async(struct mrpc_server_request_processor *request_processor);

void mrpc_server_request_processor_push_packet(struct mrpc_server_request_processor *request_processor, struct mrpc_packet *packet);

#ifdef __cplusplus
}
#endif

#endif
