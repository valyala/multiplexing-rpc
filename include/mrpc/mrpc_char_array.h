#ifndef MRPC_CHAR_ARRAY_PUBLIC
#define MRPC_CHAR_ARRAY_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_char_array;

MRPC_API struct mrpc_char_array *mrpc_char_array_create(const char *value, int len);

MRPC_API void mrpc_char_array_inc_ref(struct mrpc_char_array *char_array);

MRPC_API void mrpc_char_array_dec_ref(struct mrpc_char_array *char_array);

MRPC_API const char *mrpc_char_array_get_value(struct mrpc_char_array *char_array);

MRPC_API int mrpc_char_array_get_len(struct mrpc_char_array *char_array);

MRPC_API uint32_t mrpc_char_array_get_hash(struct mrpc_char_array *char_array, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
