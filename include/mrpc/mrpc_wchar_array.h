#ifndef MRPC_WCHAR_ARRAY_PUBLIC
#define MRPC_WCHAR_ARRAY_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_wchar_array;

MRPC_API struct mrpc_wchar_array *mrpc_wchar_array_create(const wchar_t *value, int len);

MRPC_API void mrpc_wchar_array_inc_ref(struct mrpc_wchar_array *wchar_array);

MRPC_API void mrpc_wchar_array_dec_ref(struct mrpc_wchar_array *wchar_array);

MRPC_API const wchar_t *mrpc_wchar_array_get_value(struct mrpc_wchar_array *wchar_array);

MRPC_API int mrpc_wchar_array_get_len(struct mrpc_wchar_array *wchar_array);

MRPC_API uint32_t mrpc_wchar_array_get_hash(struct mrpc_wchar_array *wchar_array, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
