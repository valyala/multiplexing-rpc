#include "private/mrpc_common.h"
#include "private/mrpc_consistent_hash.h"
#include "ff/ff_hash.h"

struct consistent_hash_entry
{
	uint32_t key;
	const void *value;
	struct consistent_hash_entry *next;
};

struct mrpc_consistent_hash
{
	struct consistent_hash_entry **buckets;
	int order;
	int uniform_factor;
	int entries_cnt;
};

static uint32_t get_bucket_num(struct mrpc_consistent_hash *consistent_hash, uint32_t key)
{
	uint32_t bucket_num;

	bucket_num = (key >> (32 - consistent_hash->order));
	return bucket_num;
}

static void add_entry_with_key(struct mrpc_consistent_hash *consistent_hash, uint32_t key, const void *value)
{
	struct consistent_hash_entry *entry;
	struct consistent_hash_entry **entry_ptr;
	struct consistent_hash_entry *new_entry;
	uint32_t bucket_num;

	bucket_num = get_bucket_num(consistent_hash, key);
	entry_ptr = &consistent_hash->buckets[bucket_num];
	entry = *entry_ptr;
	while (entry != NULL && entry->key < key)
	{
		entry_ptr = &entry->next;
		entry = *entry_ptr;;
	}
	new_entry = (struct consistent_hash_entry *) ff_malloc(sizeof(*new_entry));
	new_entry->next = entry;
	new_entry->key = key;
	new_entry->value = value;
	*entry_ptr = new_entry;
	consistent_hash->entries_cnt++;
	ff_assert(consistent_hash->entries_cnt > 0);
}

static void remove_entry_with_key(struct mrpc_consistent_hash *consistent_hash, uint32_t key)
{
	struct consistent_hash_entry *entry;
	struct consistent_hash_entry **entry_ptr;
	uint32_t bucket_num;

	bucket_num = get_bucket_num(consistent_hash, key);
	entry_ptr = &consistent_hash->buckets[bucket_num];
	entry = *entry_ptr;
	while (entry != NULL && entry->key != key)
	{
		entry_ptr = &entry->next;
		entry = *entry_ptr;
	}
	ff_assert(entry != NULL);
	ff_assert(entry->key == key);
	*entry_ptr = entry->next;
	ff_free(entry);
	consistent_hash->entries_cnt--;
	ff_assert(consistent_hash->entries_cnt >= 0);
}

struct mrpc_consistent_hash *mrpc_consistent_hash_create(int order, int uniform_factor)
{
	struct mrpc_consistent_hash *consistent_hash;
	uint32_t buckets_cnt;

	ff_assert(order >= 0);
	ff_assert(order <= 20);
	ff_assert(uniform_factor > 0);
	ff_assert(uniform_factor < 0x100);

	buckets_cnt = 1ul << order;
	consistent_hash = (struct mrpc_consistent_hash *) ff_malloc(sizeof(*consistent_hash));
	consistent_hash->buckets = (struct consistent_hash_entry **) ff_calloc(buckets_cnt, sizeof(consistent_hash->buckets[0]));
	consistent_hash->order = order;
	consistent_hash->uniform_factor = uniform_factor;
	consistent_hash->entries_cnt = 0;

	return consistent_hash;
}

void mrpc_consistent_hash_delete(struct mrpc_consistent_hash *consistent_hash)
{
	ff_assert(consistent_hash->entries_cnt == 0);

	ff_free(consistent_hash->buckets);
	ff_free(consistent_hash);
}

void mrpc_consistent_hash_remove_all_entries(struct mrpc_consistent_hash *consistent_hash)
{
	struct consistent_hash_entry **buckets;
	int buckets_cnt;
	int entries_cnt;
	int i;

	buckets_cnt = 1l << consistent_hash->order;
	entries_cnt = consistent_hash->entries_cnt;
	buckets = consistent_hash->buckets;
	for (i = 0; i < buckets_cnt; i++)
	{
		struct consistent_hash_entry *entry;

		entry = buckets[i];
		while (entry != NULL)
		{
			struct consistent_hash_entry *next_entry;

			next_entry = entry->next;
			ff_free(entry);
			entry = next_entry;
			entries_cnt--;
		}
		buckets[i] = NULL;
	}
	ff_assert(entries_cnt == 0);
	consistent_hash->entries_cnt = 0;
}

void mrpc_consistent_hash_add_entry(struct mrpc_consistent_hash *consistent_hash, uint32_t key, const void *value)
{
	int i;
	int uniform_factor;

	uniform_factor = consistent_hash->uniform_factor;
	for (i = 0; i < uniform_factor; i++)
	{
		add_entry_with_key(consistent_hash, key, value);
		key = ff_hash_uint32(key, &key, 1);
	}
}

void mrpc_consistent_hash_remove_entry(struct mrpc_consistent_hash *consistent_hash, uint32_t key)
{
	int i;
	int uniform_factor;

	ff_assert(consistent_hash->entries_cnt > 0);

	uniform_factor = consistent_hash->uniform_factor;
	for (i = 0; i < uniform_factor; i++)
	{
		remove_entry_with_key(consistent_hash, key);
		key = ff_hash_uint32(key, &key, 1);
	}
}

void mrpc_consistent_hash_get_entry(struct mrpc_consistent_hash *consistent_hash, uint32_t key, const void **value)
{
	struct consistent_hash_entry **buckets;
	uint32_t bucket_num;
	int buckets_cnt;
	int i;

	ff_assert(consistent_hash->entries_cnt > 0);

	buckets_cnt = 1l << consistent_hash->order;
	bucket_num = get_bucket_num(consistent_hash, key);
	buckets = consistent_hash->buckets;

	for (i = 0; i < buckets_cnt; i++)
	{
		struct consistent_hash_entry *entry;

		entry = buckets[bucket_num];
		while (entry != NULL)
		{
			if (entry->key >= key)
			{
				*value = entry->value;
				return;
			}
			entry = entry->next;
		}
		bucket_num++;
		if (bucket_num == buckets_cnt)
		{
			bucket_num = 0;
			key = 0;
		}
	}
	ff_assert(0);
}

int mrpc_consistent_hash_is_empty(struct mrpc_consistent_hash *consistent_hash)
{
	int is_empty;

	is_empty = (consistent_hash->entries_cnt == 0);
	return is_empty;
}
