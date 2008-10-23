#include "private/mrpc_common.h"

#include "private/mrpc_server.h"
#include "private/mrpc_interface.h"
#include "ff/ff_tcp.h"
#include "ff/ff_event.h"
#include "ff/ff_pool.h"
#include "ff/ff_stream_tcp.h"

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

		client_stream = ff_stream_tcp_create(client_tcp);
		mrpc_server_stream_processor_start(stream_processor, client_stream);
	}
	stop_all_stream_processors(server);
	ff_arch_net_addr_delete(remote_addr);

	ff_event_set(server->stop_event);
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
	if (result != FF_SUCCESS)
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

	return server;
}

void mrpc_server_delete(struct mrpc_server *server)
{
	ff_assert(server->stream_processors_cnt == 0);

	ff_pool_delete(server->stream_processors);
	ff_event_delete(server->stream_processors_stop_event);
	ff_event_delete(server->stop_event);
	ff_tcp_delete(server->accept_tcp);
	ff_free(server);
}

void mrpc_server_start(struct mrpc_server *server)
{
	ff_core_fiberpool_execute_async(main_server_func, server);
}

void mrpc_server_stop(struct mrpc_server *server)
{
	ff_tcp_disconnect(server->accept_tcp);
	ff_event_wait(server->stop_event);
	ff_assert(server->stream_processors_cnt == 0);
}
