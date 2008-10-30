#ifndef MRPC_WCHAR_ARRAY_PRIVATE
#define MRPC_WCHAR_ARRAY_PRIVATE

#include "private/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_wchar_array;

struct mrpc_wchar_array *mrpc_wchar_array_create(const wchar_t *value, int len);

void mrpc_wchar_array_inc_ref(struct mrpc_wchar_array *wchar_array);

void mrpc_wchar_array_dec_ref(struct mrpc_wchar_array *wchar_array);

const wchar_t *mrpc_wchar_array_get_value(struct mrpc_wchar_array *wchar_array);

int mrpc_wchar_array_get_len(struct mrpc_wchar_array *wchar_array);

#ifdef __cplusplus
}
#endif

#endif
