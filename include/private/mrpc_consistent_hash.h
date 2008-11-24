#ifndef MRPC_CONSISTENT_HASH_PRIVATE_H
#define MRPC_CONSISTENT_HASH_PRIVATE_H

#include "private/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_consistent_hash;

/**
 * creates the consistent hash with the given order and uniform_factor.
 * order can be from 0 to 20,
 * uniform_factor can be from 1 to 255.
 */
struct mrpc_consistent_hash *mrpc_consistent_hash_create(int order, int uniform_factor);

/**
 * Deletes the consistent hash.
 * The hash must be empty before deleting.
 */
void mrpc_consistent_hash_delete(struct mrpc_consistent_hash *consistent_hash);

/**
 * Removes all entries from the consistent_hash
 */
void mrpc_consistent_hash_remove_all_entries(struct mrpc_consistent_hash *consistent_hash);

/**
 * Adds the given entry with the given key and value to the consistent hash.
 */
void mrpc_consistent_hash_add_entry(struct mrpc_consistent_hash *consistent_hash, uint32_t key, void *value);

/**
 * Removes the entry with the given key from the consistent hash.
 * It is assumed that the entry already exists.
 */
void mrpc_consistent_hash_remove_entry(struct mrpc_consistent_hash *consistent_hash, uint32_t key);

/**
 * Returns value for the entry with the given key.
 * It is assumed that the consistent_hash contains at least one entry.
 */
void mrpc_consistent_hash_get_entry(struct mrpc_consistent_hash *consistent_hash, uint32_t key, void **value);

/**
 * Returns 1 if the consistent has is empty, otherwise returns 0.
 */
int mrpc_consistent_hash_is_empty(struct mrpc_consistent_hash *consistent_hash);

#ifdef __cplusplus
}
#endif

#endif
