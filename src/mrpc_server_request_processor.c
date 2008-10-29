#include "private/mrpc_common.h"

#include "private/mrpc_server_request_processor.h"
#include "private/mrpc_interface.h"
#include "private/mrpc_packet_stream.h"
#include "ff/ff_stream.h"
#include "ff/ff_blocking_queue.h"

struct mrpc_server_request_processor
{
	mrpc_server_request_processor_release_func release_func;
	void *release_func_ctx;
	mrpc_server_request_processor_notify_error_func notify_error_func;
	void *notify_error_func_ctx;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct mrpc_packet_stream *packet_stream;
	struct ff_stream *stream;
	uint8_t request_id;
};

static void process_request_func(void *ctx)
{
	struct mrpc_server_request_processor *request_processor;
	enum ff_result result;

	request_processor = (struct mrpc_server_request_processor *) ctx;

	mrpc_packet_stream_initialize(request_processor->packet_stream, request_processor->request_id);
	result = mrpc_data_process_remote_call(request_processor->service_interface, request_processor->service_ctx, request_processor->stream);
	mrpc_packet_stream_shutdown(request_processor->packet_stream);
	if (result != FF_SUCCESS)
	{
		request_processor->notify_error_func(request_processor->notify_error_func_ctx);
	}
	request_processor->release_func(request_processor->release_func_ctx, request_processor, request_processor->request_id);
}

struct mrpc_server_request_processor *mrpc_server_request_processor_create(mrpc_server_request_processor_release_func release_func, void *release_func_ctx,
	mrpc_server_request_processor_notify_error_func notify_error_func, void *notify_error_func_ctx,
	mrpc_packet_stream_acquire_packet_func acquire_packet_func, mrpc_packet_stream_release_packet_func release_packet_func, void *packet_func_ctx,
	struct mrpc_interface *service_interface, void *service_ctx, struct ff_blocking_queue *writer_queue)
{
	struct mrpc_server_request_processor *request_processor;

	request_processor = (struct mrpc_server_request_processor *) ff_malloc(sizeof(*request_processor));
	request_processor->release_func = release_func;
	request_processor->release_func_ctx = release_func_ctx;
	request_processor->notify_error_func = notify_error_func;
	request_processor->notify_error_func_ctx = notify_error_func_ctx;
	request_processor->service_interface = service_interface;
	request_processor->service_ctx = service_ctx;
	request_processor->packet_stream = mrpc_packet_stream_create(writer_queue, acquire_packet_func, release_packet_func, packet_func_ctx);
	request_processor->stream = mrpc_packet_stream_factory_create_stream(packet_stream);
	request_processor->request_id = 0;
	return request_processor;
}

void mrpc_server_request_processor_delete(struct mrpc_server_request_processor *request_processor)
{
	ff_stream_delete(request_processor->stream);
	/* there is no need to make the call
	 *   mrpc_packet_stream_delete(request_processor->packet_stream);
	 * here, because it was already called by ff_stream_delete()
	 */
	ff_free(request_processor);
}

void mrpc_server_request_processor_start(struct mrpc_server_request_processor *request_processor, uint8_t request_id)
{
	request_processor->request_id = request_id;
	ff_core_fiberpool_execute_async(process_request_func, request_processor);
}

void mrpc_server_request_processor_stop_async(struct mrpc_server_request_processor *request_processor)
{
	ff_stream_disconnect(request_processor->stream);
}

void mrpc_server_request_processor_push_packet(struct mrpc_server_request_processor *request_processor, struct mrpc_packet *packet)
{
	mrpc_packet_stream_push_packet(request_processor->packet_stream, packet);
}
