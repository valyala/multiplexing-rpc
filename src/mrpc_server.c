#include "private/mrpc_common.h"

#include "private/mrpc_server.h"
#include "private/mrpc_server_stream_processor.h"
#include "private/mrpc_interface.h"
#include "private/mrpc_bitmap.h"
#include "ff/ff_stream_acceptor.h"
#include "ff/ff_event.h"
#include "ff/ff_pool.h"
#include "ff/ff_core.h"

/**
 * maximum number of simultaneously working server stream processors.
 * Actually this is equivalent to the maximum number of distinct connections,
 * which the server is able to process in parallel.
 * Value of this parameter was chosen arbitrarly.
 */
#define MAX_STREAM_PROCESSORS_CNT 200

struct mrpc_server
{
	struct mrpc_interface *service_interface;
	void *service_ctx;
	struct ff_stream_acceptor *stream_acceptor;
	struct ff_event *stop_event;
	struct ff_event *stream_processors_stop_event;
	struct mrpc_bitmap *stream_processors_bitmap;
	struct ff_pool *stream_processors_pool;
	struct mrpc_server_stream_processor **active_stream_processors;
	int active_stream_processors_cnt;
};

static void stop_all_stream_processors(struct mrpc_server *server)
{
	int i;

	for (i = 0; i < MAX_STREAM_PROCESSORS_CNT; i++)
	{
		struct mrpc_server_stream_processor *stream_processor;

		stream_processor = server->active_stream_processors[i];
		if (stream_processor != NULL)
		{
			mrpc_server_stream_processor_stop_async(stream_processor);
		}
	}
	ff_event_wait(server->stream_processors_stop_event);
	ff_assert(server->active_stream_processors_cnt == 0);
}

static struct mrpc_server_stream_processor *acquire_stream_processor(struct mrpc_server *server)
{
	struct mrpc_server_stream_processor *stream_processor;
	int stream_processor_id;

	ff_assert(server->active_stream_processors_cnt >= 0);
	ff_assert(server->active_stream_processors_cnt <= MAX_STREAM_PROCESSORS_CNT);

	stream_processor = (struct mrpc_server_stream_processor *) ff_pool_acquire_entry(server->stream_processors_pool);
	stream_processor_id = mrpc_server_stream_processor_get_id(stream_processor);
	ff_assert(server->active_stream_processors[stream_processor_id] == NULL);
	server->active_stream_processors[stream_processor_id] = stream_processor;

	server->active_stream_processors_cnt++;
	ff_assert(server->active_stream_processors_cnt <= MAX_STREAM_PROCESSORS_CNT);
	if (server->active_stream_processors_cnt == 1)
	{
		ff_event_reset(server->stream_processors_stop_event);
	}

	return stream_processor;
}

static void release_stream_processor(void *ctx, struct mrpc_server_stream_processor *stream_processor)
{
	struct mrpc_server *server;
	int stream_processor_id;

	server = (struct mrpc_server *) ctx;

	ff_assert(server->active_stream_processors_cnt > 0);
	ff_assert(server->active_stream_processors_cnt <= MAX_STREAM_PROCESSORS_CNT);

	stream_processor_id = mrpc_server_stream_processor_get_id(stream_processor);
	ff_assert(server->active_stream_processors[stream_processor_id] == stream_processor);
	server->active_stream_processors[stream_processor_id] = NULL;
	ff_pool_release_entry(server->stream_processors_pool, stream_processor);

	server->active_stream_processors_cnt--;
	if (server->active_stream_processors_cnt == 0)
	{
		ff_event_set(server->stream_processors_stop_event);
	}
}

static int acquire_stream_processor_id(struct mrpc_server *server)
{
	int stream_processor_id;

	stream_processor_id = mrpc_bitmap_acquire_bit(server->stream_processors_bitmap);
	ff_assert(stream_processor_id >= 0);
	ff_assert(stream_processor_id < MAX_STREAM_PROCESSORS_CNT);

	return stream_processor_id;
}

static void release_stream_processor_id(void *ctx, int stream_processor_id)
{
	struct mrpc_server *server;

	ff_assert(stream_processor_id >= 0);
	ff_assert(stream_processor_id < MAX_STREAM_PROCESSORS_CNT);

	server = (struct mrpc_server *) ctx;
	mrpc_bitmap_release_bit(server->stream_processors_bitmap, stream_processor_id);
}

static void *create_stream_processor(void *ctx)
{
	struct mrpc_server *server;
	struct mrpc_server_stream_processor *stream_processor;
	int stream_processor_id;

	server = (struct mrpc_server *) ctx;
	stream_processor_id = acquire_stream_processor_id(server);
	stream_processor = mrpc_server_stream_processor_create(release_stream_processor, server, release_stream_processor_id, server, stream_processor_id);
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

	server = (struct mrpc_server *) ctx;

	ff_assert(server->active_stream_processors_cnt == 0);
	ff_event_set(server->stream_processors_stop_event);
	for (;;)
	{
		struct mrpc_server_stream_processor *stream_processor;
		struct ff_stream *client_stream;

		client_stream = ff_stream_acceptor_accept(server->stream_acceptor);
		if (client_stream == NULL)
		{
			ff_log_debug(L"mrpc_server_stop() has been called, so the stream_acceptor=%p of the server=%p returned NULL", server->stream_acceptor, server);
			break;
		}
		stream_processor = acquire_stream_processor(server);
		mrpc_server_stream_processor_start(stream_processor, server->service_interface, server->service_ctx, client_stream);
	}
	stop_all_stream_processors(server);

	ff_event_set(server->stop_event);
}

struct mrpc_server *mrpc_server_create()
{
	struct mrpc_server *server;

	server = (struct mrpc_server *) ff_malloc(sizeof(*server));
	server->service_interface = NULL;
	server->service_ctx = NULL;
	server->stream_acceptor = NULL;
	server->stop_event = ff_event_create(FF_EVENT_AUTO);
	server->stream_processors_stop_event = ff_event_create(FF_EVENT_AUTO);
	server->stream_processors_bitmap = mrpc_bitmap_create(MAX_STREAM_PROCESSORS_CNT);
	server->stream_processors_pool = ff_pool_create(MAX_STREAM_PROCESSORS_CNT, create_stream_processor, server, delete_stream_processor);
	server->active_stream_processors = (struct mrpc_server_stream_processor **) ff_calloc(MAX_STREAM_PROCESSORS_CNT, sizeof(server->active_stream_processors[0]));
	server->active_stream_processors_cnt = 0;

	return server;
}

void mrpc_server_delete(struct mrpc_server *server)
{
	ff_assert(server->service_interface == NULL);
	ff_assert(server->service_ctx == NULL);
	ff_assert(server->stream_acceptor == NULL);
	ff_assert(server->active_stream_processors_cnt == 0);

	ff_free(server->active_stream_processors);
	ff_pool_delete(server->stream_processors_pool);
	mrpc_bitmap_delete(server->stream_processors_bitmap);
	ff_event_delete(server->stream_processors_stop_event);
	ff_event_delete(server->stop_event);
	ff_free(server);
}

void mrpc_server_start(struct mrpc_server *server, struct mrpc_interface *service_interface, void *service_ctx, struct ff_stream_acceptor *stream_acceptor)
{
	ff_assert(server->service_interface == NULL);
	ff_assert(server->service_ctx == NULL);
	ff_assert(server->stream_acceptor == NULL);
	ff_assert(server->active_stream_processors_cnt == 0);

	ff_assert(service_interface != NULL);
	ff_assert(stream_acceptor != NULL);

	server->service_interface = service_interface;
	server->service_ctx = service_ctx;
	server->stream_acceptor = stream_acceptor;
	ff_stream_acceptor_initialize(server->stream_acceptor);
	ff_core_fiberpool_execute_async(main_server_func, server);
}

void mrpc_server_stop(struct mrpc_server *server)
{
	ff_assert(server->service_interface != NULL);
	ff_assert(server->stream_acceptor != NULL);

	ff_stream_acceptor_shutdown(server->stream_acceptor);
	ff_event_wait(server->stop_event);
	ff_assert(server->active_stream_processors_cnt == 0);

	server->service_interface = NULL;
	server->service_ctx = NULL;
	server->stream_acceptor = NULL;
}
