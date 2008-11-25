#include "private/mrpc_common.h"
#include "private/mrpc_distributed_client_wrapper.h"
#include "private/mrpc_client.h"
#include "ff/ff_stream_connector.h"
#include "ff/ff_event.h"

struct mrpc_distributed_client_wrapper
{
	struct mrpc_client *client;
	struct ff_event *stop_event;
	struct ff_stream_connector *stream_connector;
	int ref_cnt;
};

struct mrpc_distributed_client_wrapper *mrpc_distributed_client_wrapper_create()
{
	struct mrpc_distributed_client_wrapper *client_wrapper;

	client_wrapper = (struct mrpc_distributed_client_wrapper *) ff_malloc(sizeof(*client_wrapper));
	client_wrapper->client = mrpc_client_create();
	client_wrapper->stop_event = ff_event_create(FF_EVENT_MANUAL);

	client_wrapper->stream_connector = NULL;
	client_wrapper->ref_cnt = 0;

	return client_wrapper;
}

void mrpc_distributed_client_wrapper_delete(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper != NULL);
	ff_assert(client_wrapper->stream_connector == NULL);
	ff_assert(client_wrapper->ref_cnt == 0);

	ff_event_delete(client_wrapper->stop_event);
	mrpc_client_delete(client_wrapper->client);
	ff_free(client_wrapper);
}

void mrpc_distributed_client_wrapper_start(struct mrpc_distributed_client_wrapper *client_wrapper, struct ff_stream_connector *stream_connector)
{
	ff_assert(client_wrapper != NULL);
	ff_assert(stream_connector != NULL);
	ff_assert(client_wrapper->stream_connector == NULL);
	ff_assert(client_wrapper->ref_cnt == 0);

	client_wrapper->stream_connector = stream_connector;
	ff_event_set(client_wrapper->stop_event);
	mrpc_client_start(client_wrapper->client, stream_connector);
}

void mrpc_distributed_client_wrapper_stop(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper != NULL);
	ff_assert(client_wrapper->stream_connector != NULL);

	mrpc_client_stop(client_wrapper->client);
	ff_event_wait(client_wrapper->stop_event);
	ff_assert(client_wrapper->ref_cnt == 0);
	ff_stream_connector_delete(client_wrapper->stream_connector);

	client_wrapper->stream_connector = NULL;
}

struct mrpc_client *mrpc_distributed_client_wrapper_acquire_client(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper != NULL);
	ff_assert(client_wrapper->stream_connector != NULL);
	ff_assert(client_wrapper->ref_cnt >= 0);

	client_wrapper->ref_cnt++;
	if (client_wrapper->ref_cnt == 1)
	{
		ff_event_reset(client_wrapper->stop_event);
	}

	return client_wrapper->client;
}

void mrpc_distributed_client_wrapper_release_client(struct mrpc_distributed_client_wrapper *client_wrapper, struct mrpc_client *client)
{
	ff_assert(client_wrapper != NULL);
	ff_assert(client_wrapper->stream_connector != NULL);
	ff_assert(client_wrapper->ref_cnt > 0);
	ff_assert(client_wrapper->client == client);

	client_wrapper->ref_cnt--;
	if (client_wrapper->ref_cnt == 0)
	{
		ff_event_set(client_wrapper->stop_event);
	}
}
