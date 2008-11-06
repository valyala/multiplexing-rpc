#include "private/mrpc_common.h"

#include "private/mrpc_client.h"
#include "private/mrpc_client_stream_processor.h"
#include "ff/ff_stream_connector.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"
#include "ff/ff_core.h"

#define MAX_RPC_TRIES_CNT 3
#define RPC_RETRY_TIMEOUT 200

struct mrpc_client
{
	struct ff_stream_connector *stream_connector;
	struct ff_event *stop_event;
	struct ff_event *must_shutdown_event;
	struct mrpc_client_stream_processor *stream_processor;
};

static void main_client_func(void *ctx)
{
	struct mrpc_client *client;

	client = (struct mrpc_client *) ctx;

	ff_assert(client->stream_connector != NULL);
	for (;;)
	{
		struct ff_stream *stream;

		stream = ff_stream_connector_connect(client->stream_connector);
		if (stream == NULL)
		{
			/* the mrpc_client_delete() was called */
			break;
		}
		mrpc_client_stream_processor_process_stream(client->stream_processor, stream);
		ff_stream_delete(stream);
	}
	ff_event_set(client->stop_event);
}

struct mrpc_client *mrpc_client_create()
{
	struct mrpc_client *client;

	client = (struct mrpc_client *) ff_malloc(sizeof(*client));
	client->stream_connector = NULL;
	client->stop_event = ff_event_create(FF_EVENT_AUTO);
	client->must_shutdown_event = ff_event_create(FF_EVENT_MANUAL);
	client->stream_processor = mrpc_client_stream_processor_create();

	return client;
}

void mrpc_client_delete(struct mrpc_client *client)
{
	ff_assert(client->stream_connector == NULL);

	mrpc_client_stream_processor_delete(client->stream_processor);
	ff_event_delete(client->must_shutdown_event);
	ff_event_delete(client->stop_event);
	ff_free(client);
}

void mrpc_client_start(struct mrpc_client *client, struct ff_stream_connector *stream_connector)
{
	ff_assert(client->stream_connector == NULL);
	ff_assert(stream_connector != NULL);

	client->stream_connector = stream_connector;
	ff_stream_connector_initialize(client->stream_connector);
	ff_event_reset(client->must_shutdown_event);
	ff_core_fiberpool_execute_async(main_client_func, client);
}

void mrpc_client_stop(struct mrpc_client *client)
{
	ff_assert(client->stream_connector != NULL);

	ff_stream_connector_shutdown(client->stream_connector);
	ff_event_set(client->must_shutdown_event);
	mrpc_client_stream_processor_stop_async(client->stream_processor);
	ff_event_wait(client->stop_event);

	client->stream_connector = NULL;
}

enum ff_result mrpc_client_invoke_rpc(struct mrpc_client *client, struct mrpc_data *data)
{
	enum ff_result result;
	int tries_cnt = 0;

	for (;;)
	{
		tries_cnt++;
		result = mrpc_client_stream_processor_invoke_rpc(client->stream_processor, data);
		if (result == FF_SUCCESS)
		{
			break;
		}
		if (tries_cnt >= MAX_RPC_TRIES_CNT)
		{
			break;
		}
		result = ff_event_wait_with_timeout(client->must_shutdown_event, RPC_RETRY_TIMEOUT);
		if (result == FF_SUCCESS)
		{
			/* mrpc_client_stop() was called */
			result = FF_FAILURE;
			break;
		}
	}
	return result;
}
