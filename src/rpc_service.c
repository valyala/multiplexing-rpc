interface foo
{
	method bar
	{
		request
		{
			uint32 a
			int64 b
			blob c
		}
		response
		{
			int32 d
		}
	}

	method baz
	{
		request
		{
		}
		response
		{
		}
	}
}

INTERFACE ::= "interface" id "{" METHODS_LIST "}"
METHODS_LIST ::= METHOD { METHOD }
METHOD ::= "method" id "{" REQUEST_PARAMS RESPONSE_PARAMS "}"
REQUEST_PARAMS ::= "request" "{" PARAMS_LIST "}"
RESPONSE_PARAMS ::= "response" "{" PARAMS_LIST "}"
PARAMS_LIST ::= PARAM { PARAM }
PARAM ::= TYPE id
TYPE ::= "uint32" | "uint64" | "int32" | "int64" | "string" | "blob"

id = [a-z_][a-z_\d]*

static enum ff_result read_from_packet_stream(struct ff_stream *stream, void *buf, int len)
{
	struct mrpc_packet_stream *packet_stream;
	enum ff_result result;

	packet_stream = (struct mrpc_packet_stream *) ff_stream_get_ctx(stream);
	result = mrpc_packet_stream_read(packet_stream, buf, len);
	return result;
}

static enum ff_result write_to_packet_stream(struct ff_stream *stream, const void *buf, int len)
{
	struct mrpc_packet_stream *packet_stream;
	enum ff_result result;

	packet_stream = (struct mrpc_packet_stream *) ff_stream_get_ctx(stream);
	result = mrpc_packet_stream_write(packet_stream, buf, len);
	return result;
}

static enum ff_result flush_packet_stream(struct ff_stream *stream)
{
	struct mrpc_packet_stream *packet_stream;
	enum ff_result result;

	packet_stream = (struct mrpc_packet_stream *) ff_stream_get_ctx(stream);
	result = mrpc_packet_stream_flush(packet_stream);
	return result;
}

static void disconnect_packet_stream(struct ff_stream *stream)
{
	struct mrpc_packet_stream *packet_stream;

	packet_stream = (struct mrpc_packet_stream *) stream->ctx;
	mrpc_packet_stream_disconnect(packet_stream);
}

static void delete_packet_stream(struct ff_stream *stream)
{
	struct mrpc_packet_stream *packet_stream;

	packet_stream = (struct mrpc_packet_stream *) stream->ctx;
	mrpc_packet_stream_delete(packet_stream);
	ff_free(stream);
}

static struct ff_stream_vtable packet_stream_vtable =
{
	read_from_packet_stream,
	write_to_packet_stream,
	flush_packet_stream,
	disconnect_packet_stream,
	delete_packet_stream
};

struct ff_stream *ff_stream_create_from_packet_stream(struct mrpc_packet_stream *packet_stream)
{
	struct ff_stream *stream;

	stream = ff_stream_create(&packet_stream_vtable, packet_stream);

	return stream;
}

typedef void (*mrpc_request_processor_release_func)(void *release_func_ctx, struct mrpc_request_processor *request_processor, uint8_t request_id);
typedef void (*mrpc_request_processor_notify_error_func)(void *notify_error_func_ctx);

struct mrpc_request_processor
{
	mrpc_request_processor_release_func release_func;
	void *release_func_ctx;
	mrpc_request_processor_notify_error_func notify_error_func;
	void *notify_error_func_ctx;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct mrpc_packet_stream *packet_stream;
	struct ff_stream *stream;
	uint8_t request_id;
};

static void process_request_func(void *ctx)
{
	struct mrpc_request_processor *request_processor;
	enum ff_result result;

	request_processor = (struct mrpc_request_processor *) ctx;

	result = mrpc_packet_stream_initialize(request_processor->packet_stream, request_processor->request_id);
	if (result == FF_SUCCESS)
	{
		result = mrpc_data_process_next_rpc(request_processor->service_interface, request_processor->service_ctx, request_processor->stream);
		if (result == FF_FAILURE)
		{
			request_processor->notify_error_func(request_processor->notify_error_func_ctx);
		}
		mrpc_packet_stream_shutdown(request_processor->packet_stream);
	}
	else
	{
		request_processor->notify_error_func(request_processor->notify_error_func_ctx);
	}
	mrpc_packet_stream_clear_reader_queue(request_processor->packet_stream);
	request_processor->release_func(request_processor->release_func_ctx, request_processor, request_processor->request_id);
}

struct mrpc_request_processor *mrpc_request_processor_create(mrpc_request_processor_release_func release_func, void *release_func_ctx,
	mrpc_request_processor_notify_error_func notify_error_func, void *notify_error_func_ctx,
	mrpc_request_processor_acquire_packet_func acquire_packet_func, mrpc_request_processor_release_packet_func release_packet_func, void *packet_func_ctx,
	struct mrpc_interface *service_interface, void *service_ctx, struct ff_blocking_queue *writer_queue)
{
	struct mrpc_request_processor *request_processor;

	request_processor = (struct mrpc_request_processor *) ff_malloc(sizeof(*request_processor));
	request_processor->release_func = release_func;
	request_processor->release_func_ctx = release_func_ctx;
	request_processor->notify_error_func = notify_error_func;
	request_processor->notify_error_func_ctx = notify_error_func_ctx;
	request_processor->service_interface = service_interface;
	request_processor->service_ctx = service_ctx;
	request_processor->packet_stream = mrpc_packet_stream_create(writer_queue, acquire_packet_func, release_packet_func, packet_func_ctx);
	request_processor->stream = ff_stream_create_from_packet_stream(packet_stream);
	request_processor->request_id = 0;
	return request_processor;
}

void mrpc_request_processor_delete(struct mrpc_request_processor *request_processor)
{
	ff_stream_delete(request_processor->stream);
	/* there is no need to make the call
	 *   mrpc_packet_stream_delete(request_processor->packet_stream);
	 * here, because it was already called by ff_stream_delete()
	 */
	ff_free(request_processor);
}

void mrpc_request_processor_start(struct mrpc_request_processor *request_processor, uint8_t request_id)
{
	request_processor->request_id = request_id;
	ff_core_fiberpool_execute_async(process_request_func, request_processor);
}

void mrpc_request_processor_stop_async(struct mrpc_request_processor *request_processor)
{
	ff_stream_disconnect(request_processor->stream);
}

void mrpc_request_processor_push_packet(struct mrpc_request_processor *request_processor, struct mrpc_packet *packet)
{
	mrpc_packet_stream_push_packet(request_processor->packet_stream, packet);
}

#define MAX_REQUEST_STREAMS_CNT 0x100
#define MAX_WRITER_QUEUE_SIZE 1000
#define MAX_PACKETS_CNT 1000

struct mrpc_client_stream_processor
{
	struct ff_blocking_queue *writer_queue;
	struct ff_pool *request_streams_pool;
	struct mrpc_bitmap *request_streams_bitmap;
	struct ff_event *request_streams_stop_event;
	struct ff_pool *packets_pool;
	struct mrpc_request_stream *request_streams[MAX_REQUEST_STREAMS_CNT];
	struct ff_stream *stream;
	int request_streams_cnt;
};

static void *create_request_stream(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;
	struct mrpc_request_stream *request_stream;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;
	request_stream = mrpc_request_stream_create(stream_processor->writer_queue, stream_processor->request_streams_bitmap);
	return request_stream;
}

static void delete_request_stream(void *ctx)
{
	struct mrpc_request_stream *request_stream;

	request_stream = (struct mrpc_request_stream *) ctx;
	mrpc_request_stream_delete(request_stream);
}

static struct mrpc_request_stream *acquire_request_stream(struct mrpc_client_stream_processor *stream_processor)
{
	struct mrpc_request_stream *request_stream;
	uint8_t request_id;

	ff_assert(stream_processor->request_streams_cnt >= 0);

	request_stream = (struct mrpc_request_stream *) ff_pool_acquire_entry(stream_processor->request_streams_pool);
	request_id = mrpc_request_stream_get_request_id(request_stream);
	ff_assert(stream_processor->request_streams[request_id] == NULL);
	stream_processor->request_streams[request_id] = request_stream;

	stream_processor->request_streams_cnt++;
	ff_assert(stream_processor->request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);
	if (stream_processor->request_streams_cnt == 1)
	{
		ff_event_reset(stream_processor->request_streams_stop_event);
	}

	return request_stream;
}

static void release_request_stream(struct mrpc_client_stream_processor *stream_processor, struct mrpc_request_stream *request_stream)
{
	uint8_t request_id;

	ff_assert(stream_processor->request_streams_cnt > 0);
	ff_assert(stream_processor->request_streams_cnt <= MAX_REQUEST_STREAMS_CNT);

	request_id = mrpc_request_stream_get_request_id(request_stream);
	ff_assert(stream_processor->request_streams[request_id] == request_stream);
	stream_processor->request_streams[request_id] = NULL;
	ff_pool_release_entry(stream_processor->request_streams_pool, request_stream);

	stream_processor->request_streams_cnt--;
	if (stream_processor->request_streams_cnt == 0)
	{
		ff_event_set(stream_processor->request_streams_stop_event);
	}
}

static void *create_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = mrpc_packet_create();
	return packet;
}

static void delete_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = (struct mrpc_packet *) ctx;
	mrpc_packet_delete(packet);
}

static void stop_request_stream(void *entry, void *ctx, int is_acquired)
{
	struct mrpc_request_stream *request_stream;

	request_stream = (struct mrpc_request_stream *) entry;
	if (is_acquired)
	{
		mrpc_request_stream_stop_async(request_stream);
	}
}

static void stop_all_request_streams(struct mrpc_client_stream_processor *stream_processor)
{
	ff_pool_for_each_entry(stream_processor->request_streams_pool, stop_request_stream, stream_processor);
	ff_event_wait(stream_processor->request_streams_stop_event);
	ff_assert(stream_processor->request_streams_cnt == 0);
}

static void stream_writer_func(void *ctx)
{
	struct mrpc_client_stream_processor *stream_processor;

	stream_processor = (struct mrpc_client_stream_processor *) ctx;

	for (;;)
	{
		struct mrpc_packet *packet;
		int is_empty;
		enum ff_result result;

		ff_blocking_queue_get(stream_processor->writer_queue, &packet);
		if (packet == NULL)
		{
			is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
			ff_assert(is_empty);
			break;
		}
		result = mrpc_packet_write_to_stream(packet, stream_processor->stream);
		ff_pool_release_entry(stream_processor->packets_pool, packet);
		is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
		if (result == FF_SUCCESS && is_empty)
		{
			result = ff_stream_flush(stream_processor->stream);
		}
		if (result == FF_FAILURE)
		{
			mrpc_client_stream_processor_stop_async(stream_processor);
			skip_writer_queue_packets(stream_processor);
			break;
		}
	}

	ff_event_set(stream_processor->writer_stop_event);
}

static void start_stream_writer(struct mrpc_client_stream_processor *stream_processor)
{
	ff_core_fiberpool_execute_async(stream_writer_func, stream_processor);
}

static void stop_stream_writer(struct mrpc_client_stream_processor *stream_processor)
{
	ff_blocking_queue_put(stream_processor->writer_queue, NULL);
	ff_event_wait(stream_processor->writer_stop_event);
}

struct mrpc_client_stream_processor *mrpc_client_stream_processor_create()
{
	struct mrpc_client_stream_processor *stream_processor;
	int i;

	stream_processor = (struct mrpc_client_stream_processor *) ff_malloc(sizeof(*stream_processor));
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_WRITER_QUEUE_SIZE);
	stream_processor->request_streams_pool = ff_pool_create(MAX_REQUEST_STREAMS_CNT, create_request_stream, stream_processor, delete_request_stream);
	stream_processor->request_streams_bitmap = mrpc_bitmap_create(MAX_REQUEST_STREAMS_CNT);
	stream_processor->request_streams_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->writer_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);

	for (i = 0; i < MAX_REQUEST_STREAMS_CNT; i++)
	{
		stream_processor->request_streams[i] = NULL;
	}

	stream_processor->stream = NULL;
	stream_processor->request_streams_cnt = 0;

	return stream_processor;
}

void mrpc_client_stream_processor_delete(struct mrpc_client_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->request_streams_cnt == 0);

	ff_pool_delete(stream_processor->packets_pool);
	ff_event_delete(stream_processor->writer_stop_event);
	ff_event_delete(stream_processor->request_streams_stop_event);
	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_pool_delete(stream_processor->request_streams_pool);
	mrpc_bitmap_delete(stream_processor->request_streams_bitmap);
	ff_free(stream_processor);
}

void mrpc_client_stream_processor_process_stream(struct mrpc_client_stream_processor *stream_processor, struct ff_stream *stream)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->request_streams_cnt == 0);

	stream_processor->stream = stream;
	start_stream_writer(stream_processor);
	ff_event_set(stream_processor->request_streams_stop_event);

	for (;;)
	{
		struct mrpc_packet *packet;
		uint8_t request_id;
		struct mrpc_request_stream *request_stream;
		enum ff_result result;

		packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
		result = mrpc_packet_read_from_stream(packet, stream);
		if (result == FF_FAILURE)
		{
			ff_pool_release_entry(stream_processor->packets_pool, packet);
			break;
		}

		request_id = mrpc_packet_get_request_id(packet);
		request_stream = stream_processor->request_streams[request_id];
		if (request_stream == NULL)
		{
			ff_pool_release_entry(stream_processor->packets_pool, packet);
			break;
		}
		mrpc_request_stream_push_packet(request_stream, packet);
	}

	mrpc_client_stream_processor_stop_async(stream_processor);
	stop_all_request_streams(stream_processor);
	ff_assert(stream_processor->request_streams_cnt == 0);
	stop_stream_writer(stream_processor);
}

void mrpc_client_stream_processor_stop_async(struct mrpc_client_stream_processor *stream_processor)
{
	if (stream_processor->stream != NULL)
	{
		ff_stream_disconnect(stream_processor->stream);
		stream_processor->stream = NULL;
	}
}

enum ff_result mrpc_client_stream_processor_invoke_rpc(struct mrpc_client_stream_processor *stream_processor, struct mrpc_data *data)
{
	enum ff_result result = FF_FAILURE;

	if (stream_processor->stream != NULL)
	{
		struct mrpc_request_stream *request_stream;

		request_stream = acquire_request_stream(stream_processor);
		result = mrpc_request_stream_invoke_rpc(request_stream, data);
		release_request_stream(stream_processor, request_stream);
	}

	return result;
}

#define MAX_REQUEST_PROCESSORS_CNT 0x100
#define MAX_WRITER_QUEUE_SIZE 1000
#define MAX_PACKETS_CNT 1000

typedef void (*mrpc_server_stream_processor_release_func)(void *release_func_ctx, struct mrpc_server_stream_processor *stream_processor);

struct mrpc_server_stream_processor
{
	mrpc_server_stream_processor_release_func release_func;
	void *release_func_ctx;
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_event *writer_stop_event;
	struct ff_event *request_processors_stop_event;
	struct ff_pool *request_processors_pool;
	struct ff_pool *packets_pool;
	struct ff_blocking_queue *writer_queue;
	struct mrpc_request_processor *request_processors[MAX_REQUEST_PROCESSORS_CNT];
	struct ff_stream *stream;
	int request_processors_cnt;
};

static void skip_writer_queue_packets(struct mrpc_server_stream_processor *stream_processor)
{
	for (;;)
	{
		struct mrpc_packet *packet;

		ff_blocking_queue_get(stream_processor->writer_queue, &packet);
		if (packet == NULL)
		{
			int is_empty;

			is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
			ff_assert(is_empty);
			break;
		}
		ff_pool_release_entry(stream_processor->packets_pool, packet);
	}
}

static void stream_writer_func(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;

	for (;;)
	{
		struct mrpc_packet *packet;
		int is_empty;
		enum ff_result result;

		ff_blocking_queue_get(stream_processor->writer_queue, &packet);
		if (packet == NULL)
		{
			is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
			ff_assert(is_empty);
			break;
		}
		result = mrpc_packet_write_to_stream(packet, stream_processor->stream);
		ff_pool_release_entry(stream_processor->packets_pool, packet);
		is_empty = ff_blocking_queue_is_empty(stream_processor->writer_queue);
		if (result == FF_SUCCESS && is_empty)
		{
			result = ff_stream_flush(stream_processor->stream);
		}
		if (result == FF_FAILRE)
		{
			mrpc_server_stream_processor_stop_async(stream_processor);
			skip_writer_queue_packets(stream_processor);
			break;
		}
	}

	ff_event_set(stream_processor->writer_stop_event);
}

static void start_stream_writer(struct mrpc_server_stream_processor *stream_processor)
{
	ff_core_fiberpool_execute_async(stream_writer_func, stream_processor);
}

static void stop_stream_writer(struct mrpc_server_stream_processor *stream_processor)
{
	ff_blocking_queue_put(stream_processor->writer_queue, NULL);
	ff_event_wait(stream_processor->writer_stop_event);
}

static void stop_request_processor(void *entry, void *ctx, int is_acquired)
{
	struct mrpc_request_processor *request_processor;

	request_processor = (struct mrpc_request_processor *) entry;
	if (is_acquired)
	{
		mrpc_request_processor_stop_async(request_processor);
	}
}

static void stop_all_request_processors(struct mrpc_server_stream_processor *stream_processor)
{
	ff_pool_for_each_entry(stream_processor->request_processors_pool, stop_request_processor, stream_processor);
	ff_event_wait(stream_processor->request_processors_stop_event);
	ff_assert(stream_processor->request_processors_cnt == 0);
}

static struct mrpc_request_processor *acquire_request_processor(struct mrpc_server_stream_processor *stream_processor, uint8_t request_id)
{
	struct mrpc_request_processor *request_processor;

	ff_assert(stream_processor->request_processors_cnt >= 0);
	ff_assert(stream_processor->request_processors_cnt < MAX_REQUEST_PROCESSORS_CNT);

	request_processor = (struct mrpc_request_processor *) ff_pool_acquire_entry(stream_processor->request_processors_pool);
	ff_assert(stream_processor->request_processors[request_id] == NULL);
	stream_processor->request_processors[request_id] = request_processor;
	stream_processor->request_processors_cnt++;
	if (stream_processor->request_processors_cnt == 1)
	{
		ff_event_reset(stream_processor->request_processors_stop_event);
	}
	return request_processor;
}

static void release_request_processor(void *ctx, struct mrpc_request_processor *request_processor, uint8_t request_id)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;

	ff_assert(stream_processor->request_processors_cnt > 0);
	ff_assert(stream_processor->request_processors_cnt <= MAX_REQUEST_PROCESSORS_CNT);
	ff_assert(stream_processor->request_processors[request_id] == request_processor);

	ff_pool_release_entry(stream_processor->request_processors_pool, request_processor);
	stream_processor->request_processors[request_id] = NULL;
	stream_processor->request_processors_cnt--;
	if (stream_processor->request_processors_cnt == 0)
	{
		ff_event_set(stream_processor->request_processors_stop_event);
	}
}

static void notify_request_processor_error(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	mrpc_server_stream_processor_stop_async(stream_processor);
}

static struct mrpc_packet *acquire_packet(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct mrpc_packet *packet;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
	return packet;
}

static void release_packet(void *ctx, struct mrpc_packet *packet)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	mrpc_packet_reset(packet);
	ff_pool_release_entry(stream_processor->packets_pool, packet);
}

static void *create_request_processor(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	struct mrpc_request_processor *request_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	request_processor = mrpc_request_processor_create(release_request_processor, stream_processor, notify_request_processor_error, stream_processor,
		acquire_packet, release_packet, stream_processor,
		stream_processor->service_interface, stream_processor->service_ctx, stream_processor->writer_queue);
	return request_processor;
}

static void delete_request_processor(void *ctx)
{
	struct mrpc_request_processor *request_processor;

	request_processor = (struct mrpc_request_processor *) ctx;
	mrpc_request_processor_delete(request_processor);
}

static void *create_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = mrpc_packet_create();
	return packet;
}

static void delete_packet(void *ctx)
{
	struct mrpc_packet *packet;

	packet = (struct mrpc_packet *) ctx;
	mrpc_packet_delete(packet);
}

static void stream_reader_func(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;

	ff_assert(stream_processor->request_processors_cnt == 0);
	ff_assert(stream_processor->stream != NULL);

	start_stream_writer(stream_processor);
	ff_event_set(stream_processor->request_processors_stop_event);
	for (;;)
	{
		struct mrpc_packet *packet;
		enum mrpc_packet_type packet_type;
		uint8_t request_id;
		struct mrpc_request_processor *request_processor;
		enum ff_result result;

		packet = (struct mrpc_packet *) ff_pool_acquire_entry(stream_processor->packets_pool);
		result = mrpc_packet_read_from_stream(packet, stream_processor->stream);
		if (result == FF_FAILURE)
		{
			ff_pool_release_entry(stream_processor->packets_pool, packet);
			break;
		}

		packet_type = mrpc_packet_get_type(packet);
		request_id = mrpc_packet_get_request_id(packet);
		request_processor = stream_processor->request_processors[request_id];
		if (packet_type == MRPC_PACKET_FIRST || packet_type == MRPC_PACKET_SINGLE)
		{
			if (request_processor != NULL)
			{
				ff_pool_release_entry(stream_processor->packets_pool, packet);
				break;
			}

			request_processor = acquire_request_processor(stream_processor, request_id);
			mrpc_request_processor_start(request_processor, request_id);
		}
		else
		{
			/* packet_type is MRPC_PACKET_MIDDLE or MRPC_PACKET_LAST */
			if (request_processor == NULL)
			{
				ff_pool_release_entry(stream_processor->packets_pool, packet);
				break;
			}
		}
		mrpc_request_processor_push_packet(request_processor, packet);
	}
	mrpc_server_stream_processor_stop_async(stream_processor);
	stop_all_request_processors(stream_processor);
	stop_stream_writer(stream_processor);
	ff_stream_delete(stream_processor->stream);
	stream_processor->stream = NULL;
	stream_processor->release_func(release_func_ctx, stream_processor);
}

struct mrpc_server_stream_processor *mrpc_server_stream_processor_create(mrpc_server_stream_processor_release_func release_func,
	void *release_func_ctx, struct mrpc_interface *service_interface, void *service_ctx)
{
	struct mrpc_server_stream_processor *stream_processor;
	int i;

	stream_processor = (struct mrpc_server_stream_processor *) ff_malloc(sizeof(*stream_processor));
	stream_processor->release_func = release_func;
	stream_processor->release_func_ctx = release_func_ctx;
	stream_processor->service_interface = service_interface;
	stream_processor->service_ctx = service_ctx;
	stream_processor->writer_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	stream_processor->request_processors = ff_pool_create(MAX_REQUEST_PROCESSORS_CNT, create_request_processor, stream_processor, delete_request_processor);
	stream_processor->packets_pool = ff_pool_create(MAX_PACKETS_CNT, create_packet, stream_processor, delete_packet);
	stream_processor->writer_queue = ff_blocking_queue_create(MAX_WRITER_QUEUE_SIZE);

	for (i = 0; i < MAX_REQUEST_PROCESSORS_CNT; i++)
	{
		stream_processor->request_processors[i] = NULL;
	}

	stream_processor->stream = NULL;
	stream_processor->request_processors_cnt = 0;
}

void mrpc_server_stream_processor_delete(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream_processor->request_processors_cnt == 0);

	ff_blocking_queue_delete(stream_processor->writer_queue);
	ff_pool_delete(stream_processor->packets_pool);
	ff_pool_delete(stream_processor->request_processors);
	ff_event_delete(stream_processor->request_processors_stop_event);
	ff_event_delete(stream_processor->writer_stop_event);
	ff_free(stream_processor);
}

void mrpc_server_stream_processor_start(struct mrpc_server_stream_processor *stream_processor, struct ff_stream *stream)
{
	ff_assert(stream_processor->stream == NULL);
	ff_assert(stream != NULL);

	stream_processor->stream = stream;
	ff_core_fiberpool_execute_async(stream_reader_func, stream_processor);
}

void mrpc_server_stream_processor_stop_async(struct mrpc_server_stream_processor *stream_processor)
{
	ff_assert(stream_processor->stream != NULL);

	ff_stream_disconnect(stream_processor->stream);
}

#define RECONNECT_TIMEOUT 500
#define RPC_INVOKATION_TIMEOUT 2000

struct mrpc_client
{
	struct ff_arch_net_addr *service_addr;
	struct ff_event *stop_event;
	struct ff_event *must_shutdown_event;
	struct mrpc_client_stream_processor *stream_processor;
};

static void main_client_func(void *ctx)
{
	struct mrpc_client *client;

	client = (struct mrpc_client *) ctx;
	for (;;)
	{
		struct ff_tcp *service_tcp;
		enum ff_result result;

		service_tcp = ff_tcp_create();
		result = ff_tcp_connect(service_tcp, client->service_addr);
		if (result == FF_SUCCESS)
		{
			struct ff_stream *service_stream;

			service_stream = ff_stream_create_from_tcp(service_tcp);
			mrpc_client_stream_processor_process_stream(client->stream_processor, service_stream);
			ff_stream_delete(service_stream);
			/* there is no need to call ff_tcp_delete(service_tcp) here,
			 * because the ff_stream_delete(service_stream) already called this function
			 */
		}
		else
		{
			ff_tcp_delete(service_tcp);
		}

		result = ff_event_wait_with_timeout(client->must_shutdown_event, RECONNECT_TIMEOUT);
		if (result == FF_SUCCESS)
		{
			/* the mrpc_client_delete() was called */
			break;
		}
	}
	ff_event_set(client->stop_event);
}

static void start_client(struct mrpc_client *client)
{
	ff_core_fiberpool_execute_async(main_client_func, client);
}

static void stop_client(struct mrpc_client *client)
{
	ff_event_set(client->must_shutdown_event);
	mrpc_client_stream_processor_stop_async(client->stream_processor);
	ff_event_wait(client->stop_event);
}

struct mrpc_client *mrpc_client_create(struct ff_arch_net_addr *service_addr)
{
	struct mrpc_client *client;

	ff_assert(service_interface != NULL);
	ff_assert(service_addr != NULL);

	client = (struct mrpc_client *) ff_malloc(sizeof(*client));
	client->service_addr = service_addr;
	client->stop_event = ff_event_create(FF_EVENT_AUTO);
	client->must_shutdown_event = ff_event_create(FF_EVENT_AUTO);
	client->stream_processor = mrpc_client_stream_processor_create();

	start_client(client);

	return client;
}

void mrpc_client_delete(struct mrpc_client *client)
{
	ff_assert(client->service_addr != NULL);

	stop_client(client);

	mrpc_client_stream_processor_delete(client->stream_processor);
	ff_event_delete(client->must_shutdown_event);
	ff_event_delete(client->stop_event);
	ff_free(client);
}

enum ff_result mrpc_client_invoke_rpc(struct mrpc_client *client, struct mrpc_data *data)
{
	enum ff_result result;

	result = mrpc_client_stream_processor_invoke_rpc(client->stream_processor, data);
	return result;
}

#define MAX_STREAM_PROCESSORS_CNT 0x100

struct mrpc_server
{
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_tcp *accept_tcp;
	struct ff_event *stop_event;
	struct ff_event *stream_processors_stop_event;
	struct ff_pool *stream_processors;
	int stream_processors_cnt;
};

static void stop_stream_processor(void *entry, void *ctx, int is_acquired)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) entry;
	if (is_acquired)
	{
		mrpc_server_stream_processor_stop_async(stream_processor);
	}
}

static void stop_all_stream_processors(struct mrpc_server *server)
{
	ff_pool_for_each_entry(server->stream_processors, stop_stream_processor, server);
	ff_event_wait(server->stream_processors_stop_event);
	ff_assert(server->stream_processors_cnt == 0);
}

static struct mrpc_server_stream_processor *acquire_stream_processor(struct mrpc_server *server)
{
	struct mrpc_server_stream_processor *stream_processor;

	ff_assert(server->stream_processors_cnt >= 0);
	ff_assert(server->stream_processors_cnt < MAX_STREAM_PROCESSORS_CNT);

	stream_processor = (struct mrpc_server_stream_processor *) ff_pool_acquire_entry(server->stream_processors);
	server->stream_processors_cnt++;
	if (server->stream_processors_cnt == 1)
	{
		ff_event_reset(server->stream_processors_stop_event);
	}
	return stream_processor;
}

static void release_stream_processor(void *ctx, struct mrpc_server_stream_processor *stream_processor)
{
	struct mrpc_server *server;

	server = (struct mrpc_server *) ctx;

	ff_assert(server->stream_processors_cnt > 0);
	ff_assert(server->stream_processors <= MAX_STREAM_PROCESSORS_CNT);

	ff_pool_release_entry(server->stream_processors, stream_processor);
	server->stream_processors_cnt--;
	if (server->stream_processors_cnt == 0)
	{
		ff_event_set(server->stream_processors_stop_event);
	}
}

static void *create_stream_processor(void *ctx)
{
	struct mrpc_server *server;
	struct mrpc_server_stream_processor *stream_processor;

	server = (struct mrpc_server *) ctx;
	stream_processor = mrpc_server_stream_processor_create(release_stream_processor, server, server->service_interface, server->service_ctx);
	return stream_processor;
}

static void delete_stream_processor(void *ctx)
{
	struct mrpc_server_stream_processor *stream_processor;

	stream_processor = (struct mrpc_server_stream_processor *) ctx;
	mrpc_server_stream_processor_delete(stream_processor);
}

static void main_server_func(void *ctx)
{
	struct mrpc_server *server;
	struct ff_arch_net_addr *remote_addr;

	server = (struct mrpc_server *) ctx;

	ff_assert(server->stream_processors_cnt == 0);

	remote_addr = ff_arch_net_addr_create();
	ff_event_set(server->stream_processors_stop_event);
	for (;;)
	{
		struct ff_tcp *client_tcp;
		struct mrpc_server_stream_processor *stream_processor;
		struct ff_stream *client_stream;

		client_tcp = ff_tcp_accept(server->accept_tcp, remote_addr);
		if (client_tcp == NULL)
		{
			break;
		}

		stream_processor = acquire_stream_processor(server);

		client_stream = ff_stream_create_from_tcp(client_tcp);
		mrpc_server_stream_processor_start(stream_processor, client_stream);
	}
	stop_all_stream_processors(server);
	ff_arch_net_addr_delete(remote_addr);

	ff_event_set(server->stop_event);
}

static void start_server(struct mrpc_server *server)
{
	ff_core_fiberpool_execute_async(main_server_func, server);
}

static void stop_server(struct mrpc_server *server)
{
	ff_tcp_disconnect(server->accept_tcp);
	ff_event_wait(server->stop_event);
	ff_assert(server->stream_processors_cnt == 0);
}

struct mrpc_server *mrpc_server_create(struct mrpc_interface *service_interface, void *service_ctx, struct ff_arch_net_addr *listen_addr)
{
	struct mrpc_server *server;
	enum ff_result result;

	server = (struct mrpc_server *) ff_malloc(sizeof(*server));
	server->service_interface = service_interface;
	server->service_ctx = service_ctx;
	server->accept_tcp = ff_tcp_create();
	result = ff_tcp_bind(server->accept_tcp, params->listen_addr, FF_TCP_SERVER);
	if (result == FF_FAILURE)
	{
		const wchar_t *addr_str;

		addr_str = ff_arch_net_addr_to_string(params->listen_addr);
		ff_log_fatal_error(L"cannot bind the address %ls to the server", addr_str);
		/* there is no need to call ff_arch_net_add_delete_string() here,
		 * because the ff_log_fatal_error() will terminate the application
		 */
	}
	server->stop_event = ff_event_create(FF_EVENT_AUTO);
	server->stream_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	server->stream_processors = ff_pool_create(MAX_STREAM_PROCESSORS_CNT, create_stream_processor, server, delete_stream_processor);
	server->stream_processors_cnt = 0;

	start_server(server);

	return server;
}

void mrpc_server_delete(struct mrpc_server *server)
{
	stop_server(server);

	ff_pool_delete(server->stream_processors);
	ff_event_delete(server->stream_processors_stop_event);
	ff_event_delete(server->stop_event);
	ff_tcp_delete(server->accept_tcp);
	ff_free(server);
}

foo_interface = foo_interface_create();
service_ctx = foo_service_create();

server = mrpc_server_create(foo_interface, service_ctx, listen_addr);
wait();

mrpc_server_delete(server);
mrpc_interface_delete(foo_interface);


static const mrpc_param_constructor foo_request_param_constructors[] =
{
	mrpc_uint32_param_create,
	mrpc_int64_param_create,
	mrpc_blob_param_create,
	NULL
};

static const mrpc_param_constructor foo_response_param_constructors[] =
{
	mrpc_int32_param_create,
	NULL
};

static void foo_callback(struct mrpc_data *data, void *service_ctx)
{
	uint32_t a;
	int64_t b;
	struct mrpc_blob *c;
	int32_t d;

	mrpc_data_get_request_param_value(data, 0, &a);
	mrpc_data_get_request_param_value(data, 1, &b);
	mrpc_data_get_request_param_value(data, 2, &c);

	service_func_foo(service_ctx, a, b, c, &d);

	mrpc_data_set_response_param_value(data, 0, &d);
}

static struct mrpc_method *create_foo_method()
{
	struct mrpc_method *method;

	method = mrpc_method_create_server(foo_request_param_constructors, foo_response_param_constructors, foo_callback);
	return method;
}

static const mrpc_method_constructor foo_method_constructors[] =
{
	create_foo_method,
	NULL
}

struct mrpc_interface *foo_interface_create()
{
	struct mrpc_interface *interface;

	interface = mrpc_interface_create(foo_method_constructors);
	return interface;
}
