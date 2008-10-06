#ifndef MRPC_STRING_PARAM_PUBLIC
#define MRPC_STRING_PARAM_PUBLIC

#include "mrpc/mrpc_param_vtable.h"

#ifdef __cplusplus
extern "C" {
#endif

MRPC_API const struct mrpc_param_vtable *mrpc_string_param_get_vtable();

#ifdef __cplusplus
}
#endif

#endif
