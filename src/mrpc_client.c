#include "private/mrpc_common.h"

#include "private/mrpc_client.h"
#include "private/mrpc_interface.h"
#include "private/mrpc_client_stream_processor.h"
#include "private/mrpc_data.h"
#include "ff/ff_stream_connector.h"
#include "ff/ff_stream.h"
#include "ff/ff_event.h"
#include "ff/ff_core.h"

struct mrpc_client
{
	struct ff_event *stop_event;
	struct mrpc_client_stream_processor *stream_processor;
	const struct mrpc_interface *client_interface;
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

	client->client_interface = NULL;
	client->stream_connector = NULL;

	return client;
}

void mrpc_client_delete(struct mrpc_client *client)
{
	ff_assert(client != NULL);
	ff_assert(client->client_interface == NULL);
	ff_assert(client->stream_connector == NULL);

	mrpc_client_stream_processor_delete(client->stream_processor);
	ff_event_delete(client->stop_event);
	ff_free(client);
}

void mrpc_client_start(struct mrpc_client *client, const struct mrpc_interface *client_interface, struct ff_stream_connector *stream_connector)
{
	ff_assert(client != NULL);
	ff_assert(client_interface != NULL);
	ff_assert(stream_connector != NULL);
	ff_assert(client->client_interface == NULL);
	ff_assert(client->stream_connector == NULL);

	client->client_interface = client_interface;
	client->stream_connector = stream_connector;
	ff_stream_connector_initialize(client->stream_connector);
	ff_core_fiberpool_execute_async(main_client_func, client);
}

void mrpc_client_stop(struct mrpc_client *client)
{
	ff_assert(client != NULL);
	ff_assert(client->client_interface != NULL);
	ff_assert(client->stream_connector != NULL);

	ff_stream_connector_shutdown(client->stream_connector);
	mrpc_client_stream_processor_stop_async(client->stream_processor);
	ff_event_wait(client->stop_event);

	client->client_interface = NULL;
	client->stream_connector = NULL;
}

struct mrpc_data *mrpc_client_create_data(struct mrpc_client *client, uint8_t method_id)
{
	struct mrpc_data *data;

	ff_assert(client != NULL);
	ff_assert(client->client_interface != NULL);

	data = mrpc_data_create(client->client_interface, method_id);
	ff_assert(data != NULL);
	return data;
}

enum ff_result mrpc_client_invoke_rpc(struct mrpc_client *client, struct mrpc_data *data)
{
	enum ff_result result;

	ff_assert(client != NULL);
	ff_assert(data != NULL);

	result = mrpc_client_stream_processor_invoke_rpc(client->stream_processor, data);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot invoke rpc (data=%p) on the client=%p. See previous messages for more info", data, client);
	}
	return result;
}
