#ifndef MRPC_BLOB_PARAM_PUBLIC
#define MRPC_BLOB_PARAM_PUBLIC

#include "mrpc/mrpc_param_vtable.h"

#ifdef __cplusplus
extern "C" {
#endif

MRPC_API const struct mrpc_param_vtable *mrpc_blob_param_get_vtable();

#ifdef __cplusplus
}
#endif

#endif
