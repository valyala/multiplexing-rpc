#include "private/mrpc_common.h"

#include "private/mrpc_client.h"
#include "private/mrpc_client_stream_processor.h"
#include "ff/ff_stream_connector.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"
#include "ff/ff_core.h"

/**
 * the number of milliseconds between tries on the mrpc_client_create_request_stream()
 */
#define CREATE_REQUEST_STREAM_TRY_INTERVAL 100

struct mrpc_client
{
	struct ff_event *stop_event;
	struct mrpc_client_stream_processor *stream_processor;
	struct ff_stream_connector *stream_connector;
};

static void main_client_func(void *ctx)
{
	struct mrpc_client *client;

	client = (struct mrpc_client *) ctx;

	ff_assert(client != NULL);
	ff_assert(client->stream_connector != NULL);
	for (;;)
	{
		struct ff_stream *stream;

		stream = ff_stream_connector_connect(client->stream_connector);
		if (stream == NULL)
		{
			/* the mrpc_client_delete() was called */
			ff_log_debug(L"cannot establish connection for the client=%p, because the mrpc_client_stop() has been called", client);
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
	client->stop_event = ff_event_create(FF_EVENT_AUTO);
	client->stream_processor = mrpc_client_stream_processor_create();

	client->stream_connector = NULL;

	return client;
}

void mrpc_client_delete(struct mrpc_client *client)
{
	ff_assert(client != NULL);
	ff_assert(client->stream_connector == NULL);

	mrpc_client_stream_processor_delete(client->stream_processor);
	ff_event_delete(client->stop_event);
	ff_free(client);
}

void mrpc_client_start(struct mrpc_client *client, struct ff_stream_connector *stream_connector)
{
	ff_assert(client != NULL);
	ff_assert(stream_connector != NULL);
	ff_assert(client->stream_connector == NULL);

	client->stream_connector = stream_connector;
	ff_stream_connector_initialize(client->stream_connector);
	ff_core_fiberpool_execute_async(main_client_func, client);
}

void mrpc_client_stop(struct mrpc_client *client)
{
	ff_assert(client != NULL);
	ff_assert(client->stream_connector != NULL);

	ff_stream_connector_shutdown(client->stream_connector);
	mrpc_client_stream_processor_stop_async(client->stream_processor);
	ff_event_wait(client->stop_event);

	client->stream_connector = NULL;
}

struct ff_stream *mrpc_client_create_request_stream(struct mrpc_client *client, int tries_cnt)
{
	struct ff_stream *stream;

	ff_assert(client != NULL);
	ff_assert(tries_cnt > 0);

	for (;;)
	{
		stream = mrpc_client_stream_processor_create_request_stream(client->stream_processor);
		if (stream != NULL)
		{
			break;
		}
		ff_log_debug(L"the client=%p cannot acquire request stream. See previous messages for more info");
		tries_cnt--;
		if (tries_cnt == 0)
		{
			break;
		}
		ff_core_sleep(CREATE_REQUEST_STREAM_TRY_INTERVAL);
	}
	return stream;
}

void mrpc_client_reset_connection(struct mrpc_client *client)
{
	ff_assert(client != NULL);

	mrpc_client_stream_processor_stop_async(client->stream_processor);
}
