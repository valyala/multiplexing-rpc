#include "private/mrpc_common.h"
#include "private/mrpc_distributed_client.h"
#include "private/mrpc_consistent_hash.h"
#include "private/mrpc_distributed_client_wrapper.h"
#include "private/mrpc_distributed_client_controller.h"
#include "ff/ff_dictionary.h"
#include "ff/ff_hash.h"
#include "ff/ff_core.h"
#include "ff/ff_event.h"
#include "ff/ff_stream_connector.h"

#define CONSISTENT_HASH_UNIFORM_FACTOR_ORDER 7

#define CONSISTENT_HASH_UNIFORM_FACTOR (1l << CONSISTENT_HASH_UNIFORM_FACTOR_ORDER)

#define ACQUIRE_CLIENT_TRY_SLEEP_TIMEOUT 100

#define ACQUIRE_CLIENT_MAX_TRIES_CNT 3

#define U64_HASH_START_VALUE 0

struct mrpc_distributed_client
{
	struct ff_dictionary *clients_map;
	struct mrpc_consistent_hash *consistent_hash;
	struct ff_event *stop_event;
	struct mrpc_distributed_client_controller *controller;
	int clients_cnt;
};

static uint32_t get_u64_hash(uint64_t key)
{
	uint32_t hash_value;
	uint32_t tmp[2];

	tmp[0] = (uint32_t) key;
	tmp[1] = (uint32_t) (key >> 32);
	hash_value = ff_hash_uint32(U64_HASH_START_VALUE, tmp, 2);
	return hash_value;
}

static uint32_t get_client_wrapper_key_hash(const void *key)
{
	uint64_t *entry_key;
	uint32_t hash_value;

	entry_key = (uint64_t *) key;
	hash_value = get_u64_hash(*entry_key);
	return hash_value;
}

static int is_client_wrapper_equal_keys(const void *key1, const void *key2)
{
	uint64_t *entry_key1;
	uint64_t *entry_key2;
	int is_equal;

	entry_key1 = (uint64_t *) key1;
	entry_key2 = (uint64_t *) key2;
	is_equal = (*entry_key1 == *entry_key2);
	return is_equal;
}

static void remove_client_wrapper_entry(const void *key, const void *value, void *ctx)
{
	uint64_t *entry_key;
	struct mrpc_distributed_client_wrapper *client_wrapper;
	struct mrpc_distributed_client *distributed_client;

	entry_key = (uint64_t *) key;
	client_wrapper = (struct mrpc_distributed_client_wrapper *) value;
	distributed_client = (struct mrpc_distributed_client *) ctx;

	ff_free(entry_key);
	mrpc_distributed_client_wrapper_stop(client_wrapper);
	mrpc_distributed_client_wrapper_delete(client_wrapper);
	distributed_client->clients_cnt--;
}

static void remove_all_clients(struct mrpc_distributed_client *distributed_client)
{
	ff_assert(distributed_client != NULL);
	ff_assert(distributed_client->controller != NULL);

	mrpc_consistent_hash_remove_all_entries(distributed_client->consistent_hash);
	ff_dictionary_remove_all_entries(distributed_client->clients_map, remove_client_wrapper_entry, distributed_client);
	ff_assert(distributed_client->clients_cnt == 0);
}

static void add_client(struct mrpc_distributed_client *distributed_client, struct ff_stream_connector *stream_connector, uint64_t key)
{
	struct mrpc_distributed_client_wrapper *client_wrapper;
	uint64_t *entry_key;
	enum ff_result result;

	ff_assert(distributed_client != NULL);
	ff_assert(stream_connector != NULL);
	ff_assert(distributed_client->controller != NULL);

	entry_key = ff_malloc(sizeof(*entry_key));
	*entry_key = key;
	client_wrapper = mrpc_distributed_client_wrapper_create();
	mrpc_distributed_client_wrapper_start(client_wrapper, stream_connector);
	result = ff_dictionary_add_entry(distributed_client->clients_map, entry_key, client_wrapper);
	if (result == FF_SUCCESS)
	{
		uint32_t consistent_hash_key;

		consistent_hash_key = get_u64_hash(key);
		mrpc_consistent_hash_add_entry(distributed_client->consistent_hash, consistent_hash_key, client_wrapper);
		distributed_client->clients_cnt++;
	}
	else
	{
		ff_log_warning(L"the client with key=%llu and has been already registered in the distributed_client=%p", key, distributed_client);
		mrpc_distributed_client_wrapper_stop(client_wrapper);
		mrpc_distributed_client_wrapper_delete(client_wrapper);
		ff_free(entry_key);
	}
}

static void remove_client(struct mrpc_distributed_client *distributed_client, uint64_t key)
{
	struct mrpc_distributed_client_wrapper *client_wrapper;
	uint64_t *entry_key;
	enum ff_result result;

	ff_assert(distributed_client != NULL);
	ff_assert(distributed_client->controller != NULL);

	result = ff_dictionary_remove_entry(distributed_client->clients_map, &key, (const void **) &entry_key, (const void **) &client_wrapper);
	if (result == FF_SUCCESS)
	{
		uint32_t consistent_hash_key;

		ff_assert(key == *entry_key);
		ff_free(entry_key);
		consistent_hash_key = get_u64_hash(key);
		mrpc_consistent_hash_remove_entry(distributed_client->consistent_hash, consistent_hash_key);
		mrpc_distributed_client_wrapper_stop(client_wrapper);
		mrpc_distributed_client_wrapper_delete(client_wrapper);
		distributed_client->clients_cnt--;
	}
	else
	{
		ff_log_warning(L"there is no client with the key=%llu in the distributed_client=%p", key, distributed_client);
	}
}

static void distributed_client_main_func(void *ctx)
{
	struct mrpc_distributed_client *distributed_client;
	struct mrpc_distributed_client_controller *controller;

	distributed_client = (struct mrpc_distributed_client *) ctx;
	ff_assert(distributed_client != NULL);
	ff_assert(distributed_client->controller != NULL);

	controller = distributed_client->controller;
	for (;;)
	{
		struct ff_stream_connector *stream_connector;
		uint64_t key;
		enum mrpc_distributed_client_controller_message_type message_type;

		message_type = mrpc_distributed_client_controller_get_next_message(controller, &stream_connector, &key);
		switch (message_type)
		{
		case MRPC_DISTRIBUTED_CLIENT_ADD_CLIENT:
			add_client(distributed_client, stream_connector, key);
			break;
		case MRPC_DISTRIBUTED_CLIENT_REMOVE_CLIENT:
			remove_client(distributed_client, key);
			break;
		case MRPC_DISTRIBUTED_CLIENT_REMOVE_ALL_CLIENTS:
			remove_all_clients(distributed_client);
			break;
		case MRPC_DISTRIBUTED_CLIENT_STOP:
			goto end;
		}
	}

end:
	remove_all_clients(distributed_client);
	ff_event_set(distributed_client->stop_event);
}

struct mrpc_distributed_client *mrpc_distributed_client_create(int expected_clients_order)
{
	struct mrpc_distributed_client *distributed_client;
	int consistent_hash_order;

	ff_assert(expected_clients_order >= 0);

	consistent_hash_order = expected_clients_order + CONSISTENT_HASH_UNIFORM_FACTOR_ORDER;
	ff_assert(consistent_hash_order <= 20);

	distributed_client = (struct mrpc_distributed_client *) ff_malloc(sizeof(*distributed_client));
	distributed_client->clients_map = ff_dictionary_create(expected_clients_order, get_client_wrapper_key_hash, is_client_wrapper_equal_keys);
	distributed_client->consistent_hash = mrpc_consistent_hash_create(consistent_hash_order, CONSISTENT_HASH_UNIFORM_FACTOR);
	distributed_client->stop_event = ff_event_create(FF_EVENT_AUTO);

	distributed_client->controller = NULL;
	distributed_client->clients_cnt = 0;

	return distributed_client;
}

void mrpc_distributed_client_delete(struct mrpc_distributed_client *distributed_client)
{
	ff_assert(distributed_client != NULL);
	ff_assert(distributed_client->controller == NULL);
	ff_assert(distributed_client->clients_cnt == 0);

	ff_event_delete(distributed_client->stop_event);
	mrpc_consistent_hash_delete(distributed_client->consistent_hash);
	ff_dictionary_delete(distributed_client->clients_map);
	ff_free(distributed_client);
}

void mrpc_distributed_client_start(struct mrpc_distributed_client *distributed_client, struct mrpc_distributed_client_controller *controller)
{
	ff_assert(distributed_client != NULL);
	ff_assert(controller != NULL);
	ff_assert(distributed_client->controller == NULL);
	ff_assert(distributed_client->clients_cnt == 0);

	distributed_client->controller = controller;
	mrpc_distributed_client_controller_initialize(controller);
	ff_core_fiberpool_execute_async(distributed_client_main_func, distributed_client);
}

void mrpc_distributed_client_stop(struct mrpc_distributed_client *distributed_client)
{
	ff_assert(distributed_client != NULL);
	ff_assert(distributed_client->controller != NULL);

	mrpc_distributed_client_controller_shutdown(distributed_client->controller);
	ff_event_wait(distributed_client->stop_event);
	ff_assert(distributed_client->clients_cnt == 0);

	distributed_client->controller = NULL;
}

struct mrpc_client *mrpc_distributed_client_acquire_client(struct mrpc_distributed_client *distributed_client, uint32_t request_hash_value, const void **cookie)
{
	struct mrpc_client *client = NULL;
	struct mrpc_consistent_hash *consistent_hash;
	int tries_cnt = ACQUIRE_CLIENT_MAX_TRIES_CNT;

	ff_assert(distributed_client != NULL);
	ff_assert(distributed_client->controller != NULL);

	consistent_hash = distributed_client->consistent_hash;
	for (;;)
	{
		int is_empty;

		is_empty = mrpc_consistent_hash_is_empty(consistent_hash);
		if (!is_empty)
		{
			struct mrpc_distributed_client_wrapper *client_wrapper = NULL;

			mrpc_consistent_hash_get_entry(distributed_client->consistent_hash, request_hash_value, (const void **) &client_wrapper);
			ff_assert(client_wrapper != NULL);
			client = mrpc_distributed_client_wrapper_acquire_client(client_wrapper);
			*cookie = client_wrapper;
			break;
		}
		ff_log_warning(L"there are no clients registered in the distributed_client=%p", distributed_client);
		tries_cnt--;
		if (tries_cnt == 0)
		{
			break;
		}
		ff_core_sleep(ACQUIRE_CLIENT_TRY_SLEEP_TIMEOUT);
	}

	return client;
}

void mrpc_distributed_client_release_client(struct mrpc_distributed_client *distributed_client, struct mrpc_client *client, const void *cookie)
{
	struct mrpc_distributed_client_wrapper *client_wrapper;

	ff_assert(distributed_client != NULL);
	ff_assert(client != NULL);
	ff_assert(cookie != NULL);

	client_wrapper = (struct mrpc_distributed_client_wrapper *) cookie;
	mrpc_distributed_client_wrapper_release_client(client_wrapper, client);
}
