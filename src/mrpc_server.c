#include "private/mrpc_common.h"

#include "private/mrpc_server.h"
#include "private/mrpc_interface.h"
#include "ff/ff_endpoint.h"
#include "ff/ff_event.h"
#include "ff/ff_pool.h"
#include "ff/ff_log.h"

#define MAX_STREAM_PROCESSORS_CNT 0x100

struct mrpc_server
{
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_endpoint *endpoint;
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
	enum ff_result result;

	server = (struct mrpc_server *) ctx;

	ff_assert(server->stream_processors_cnt == 0);
	ff_event_set(server->stream_processors_stop_event);
	result = ff_endpoint_initialize(server->endpoint);
	if (result != FF_SUCCESS)
	{
		ff_log_fatal_error(L"cannot initialize the service endpoint");
	}
	for (;;)
	{
		struct mrpc_server_stream_processor *stream_processor;
		struct ff_stream *client_stream;

		client_stream = ff_endpoint_accept(server->endpoint);
		if (client_stream == NULL)
		{
			break;
		}
		stream_processor = acquire_stream_processor(server);
		mrpc_server_stream_processor_start(stream_processor, client_stream);
	}
	stop_all_stream_processors(server);

	ff_event_set(server->stop_event);
}

struct mrpc_server *mrpc_server_create(struct mrpc_interface *service_interface, void *service_ctx, struct ff_endpoint *endpoint)
{
	struct mrpc_server *server;
	enum ff_result result;

	server = (struct mrpc_server *) ff_malloc(sizeof(*server));
	server->service_interface = service_interface;
	server->service_ctx = service_ctx;
	server->endpoint = endpoint;
	server->stop_event = ff_event_create(FF_EVENT_AUTO);
	server->stream_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	server->stream_processors = ff_pool_create(MAX_STREAM_PROCESSORS_CNT, create_stream_processor, server, delete_stream_processor);
	server->stream_processors_cnt = 0;

	return server;
}

void mrpc_server_delete(struct mrpc_server *server)
{
	ff_assert(server->stream_processors_cnt == 0);

	ff_pool_delete(server->stream_processors);
	ff_event_delete(server->stream_processors_stop_event);
	ff_event_delete(server->stop_event);
	ff_endpoint_delete(server->endpoint);
	ff_free(server);
}

void mrpc_server_start(struct mrpc_server *server)
{
	ff_core_fiberpool_execute_async(main_server_func, server);
}

void mrpc_server_stop(struct mrpc_server *server)
{
	ff_endpoint_shutdown(server->endpoint);
	ff_event_wait(server->stop_event);
	ff_assert(server->stream_processors_cnt == 0);
}
