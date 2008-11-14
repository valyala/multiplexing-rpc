#ifndef MRPC_BLOB_PARAM_PUBLIC
#define MRPC_BLOB_PARAM_PUBLIC

#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the mrpc_param structure for the mrpc_blob.
 * Always returns correct result.
 * This function can be used only as mrpc_param_constructor,
 * which must be passed to the mrpc_method_create_server_method() or mrpc_method_create_client_method()
 * in order to create corresponding method.
 */
MRPC_API struct mrpc_param *mrpc_blob_param_create();

#ifdef __cplusplus
}
#endif

#endif
