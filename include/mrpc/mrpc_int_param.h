#ifndef MRPC_INT_PARAM_PUBLIC
#define MRPC_INT_PARAM_PUBLIC

#include "mrpc/mrpc_param_vtable.h"

#ifdef __cplusplus
extern "C" {
#endif

MRPC_API const struct mrpc_param_vtable *mrpc_uint32_param_get_vtable();

MRPC_API const struct mrpc_param_vtable *mrpc_uint64_param_get_vtable();

MRPC_API const struct mrpc_param_vtable *mrpc_int32_param_get_vtable();

MRPC_API const struct mrpc_param_vtable *mrpc_int64_param_get_vtable();

#ifdef __cplusplus
}
#endif

#endif
