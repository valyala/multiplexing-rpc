#ifndef MRPC_CLIENT_REQUEST_PROCESSOR_PRIVATE
#define MRPC_CLIENT_REQUEST_PROCESSOR_PRIVATE

#include "private/mrpc_common.h"
#include "private/mrpc_packet.h"
#include "private/mrpc_packet_stream.h"
#include "ff/ff_blocking_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_client_request_processor;

typedef void (*mrpc_client_request_processor_release_request_id_func)(void *request_id_func_ctx, uint8_t request_id);

struct mrpc_client_request_processor *mrpc_client_request_processor_create(mrpc_packet_stream_acquire_packet_func acquire_packet_func,
	mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx,
	mrpc_client_request_processor_release_request_id_func release_request_id_func, void *request_id_func_ctx,
	struct ff_blocking_queue *writer_queue, uint8_t request_id);

void mrpc_client_request_processor_delete(struct mrpc_client_request_processor *request_processor);

uint8_t mrpc_client_request_processor_get_request_id(struct mrpc_client_request_processor *request_processor);

void mrpc_client_request_processor_stop_async(struct mrpc_client_request_processor *request_processor);

void mrpc_client_request_processor_push_packet(struct mrpc_client_request_processor *request_processor, struct mrpc_packet *packet);

enum ff_result mrpc_client_request_processor_invoke_rpc(struct mrpc_client_request_processor *request_processor, struct mrpc_data *data);

#ifdef __cplusplus
}
#endif

#endif
