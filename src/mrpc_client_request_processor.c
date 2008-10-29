#include "private/mrpc_common.h"

#include "private/mrpc_client_request_processor.h"
#include "private/mrpc_packet.h"
#include "private/mrpc_packet_stream.h"
#include "ff/ff_stream.h"
#include "ff/ff_blocking_queue.h"

struct mrpc_client_request_processor
{
	struct mrpc_packet_stream *packet_stream;
	struct ff_stream *stream;
	uint8_t request_id;
};

struct mrpc_client_request_processor *mrpc_client_request_processor_create(mrpc_packet_stream_acquire_packet_func acquire_packet_func,
	mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx,
	struct ff_blocking_queue *writer_queue, uint8_t request_id)
{
	struct mrpc_client_request_processor *request_processor;

	request_processor = (struct mrpc_client_request_processor *) ff_malloc(sizeof(*request_processor));
	request_processor->packet_stream = mrpc_packet_stream_create(writer_queue, acquire_packet_func, release_packet_func, packet_func_ctx);
	request_processor->stream = mrpc_packet_stream_factory_create_stream(packet_stream);
	request_processor->request_id = request_id;

	return request_processor;
}

void mrpc_client_request_processor_delete(struct mrpc_client_request_processor *request_processor)
{
	ff_stream_delete(request_processor->stream);
	/* there is no need to make the call
	 *   mrpc_packet_stream_delete(request_processor->packet_stream);
	 * here, because it was already called by ff_stream_delete()
	 */
	ff_free(request_processor);
}

uint8_t mrpc_client_request_processor_get_request_id(struct mrpc_client_request_processor *request_processor)
{
	return request_processor->request_id;
}

void mrpc_client_request_processor_stop_async(struct mrpc_client_request_processor *request_processor)
{
	ff_stream_disconnect(request_processor->stream);
}

void mrpc_client_request_processor_push_packet(struct mrpc_client_request_processor *request_processor, struct mrpc_packet *packet)
{
	mrpc_packet_stream_push_packet(request_processor->packet_stream, packet);
}

enum ff_result mrpc_client_request_processor_invoke_rpc(struct mrpc_client_request_processor *request_processor, struct mrpc_data *data)
{
	enum ff_result result;

	mrpc_packet_stream_initialize(request_processor->packet_stream, request_processor->request_id);
	result = mrpc_data_invoke_remote_call(data, request_processor->stream);
	mrpc_packet_stream_shutdown(request_processor->packet_stream);

	return result;
}
