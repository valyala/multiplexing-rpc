#include "private/mrpc_common.h"
#include "private/mrpc_distributed_client_wrapper.h"
#include "private/mrpc_client.h"
#include "ff/ff_stream_connector.h"

struct mrpc_distributed_client_wrapper
{
	struct mrpc_client *client;
	int ref_cnt;
};

static struct mrpc_distributed_client_wrapper *create_client_wrapper()
{
	struct mrpc_distributed_client_wrapper *client_wrapper;

	client_wrapper = (struct mrpc_distributed_client_wrapper *) ff_malloc(sizeof(*client_wrapper));
	client_wrapper->client = mrpc_client_create();
	client_wrapper->ref_cnt = 1;

	return client_wrapper;
}

static void delete_client_wrapper(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper->ref_cnt == 0);
	mrpc_client_delete(client_wrapper->client);
	ff_free(client_wrapper);
}

struct mrpc_distributed_client_wrapper *mrpc_distributed_client_wrapper_create()
{
	struct mrpc_distributed_client_wrapper *client_wrapper;

	client_wrapper = create_client_wrapper();
	return client_wrapper;
}

void mrpc_distributed_client_wrapper_inc_ref(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper->ref_cnt > 0);
	client_wrapper->ref_cnt++;
}

void mrpc_distributed_client_wrapper_dec_ref(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper->ref_cnt > 0);
	client_wrapper->ref_cnt--;
	if (client_wrapper->ref_cnt == 0)
	{
		delete_client_wrapper(client_wrapper);
	}
}

void mrpc_distributed_client_wrapper_start(struct mrpc_distributed_client_wrapper *client_wrapper, struct ff_stream_connector *stream_connector)
{
	mrpc_client_start(client_wrapper->client, stream_connector);
}

void mrpc_distributed_client_wrapper_stop(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	mrpc_client_stop(client_wrapper->client);
}

struct mrpc_client *mrpc_distributed_client_wrapper_get_client(struct mrpc_distributed_client_wrapper *client_wrapper)
{
	ff_assert(client_wrapper->ref_cnt > 0);
	return client_wrapper->client;
}
