#ifndef MRPC_INT_PARAM_PUBLIC_H
#define MRPC_INT_PARAM_PUBLIC_H

#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the mrpc_param structure for the uint64_t.
 * Always returns correct result.
 * This function can be used only as mrpc_param_constructor,
 * which must be passed to the mrpc_method_create_server_method() or mrpc_method_create_client_method()
 * in order to create corresponding method.
 */
MRPC_API struct mrpc_param *mrpc_uint64_param_create();

/**
 * Returns the mrpc_param structure for the int64_t.
 * Always returns correct result.
 * This function can be used only as mrpc_param_constructor,
 * which must be passed to the mrpc_method_create_server_method() or mrpc_method_create_client_method()
 * in order to create corresponding method.
 */
MRPC_API struct mrpc_param *mrpc_int64_param_create();

/**
 * Returns the mrpc_param structure for the uint32_t.
 * Always returns correct result.
 * This function can be used only as mrpc_param_constructor,
 * which must be passed to the mrpc_method_create_server_method() or mrpc_method_create_client_method()
 * in order to create corresponding method.
 */
MRPC_API struct mrpc_param *mrpc_uint32_param_create();

/**
 * Returns the mrpc_param structure for the int32_t.
 * Always returns correct result.
 * This function can be used only as mrpc_param_constructor,
 * which must be passed to the mrpc_method_create_server_method() or mrpc_method_create_client_method()
 * in order to create corresponding method.
 */
MRPC_API struct mrpc_param *mrpc_int32_param_create();

#ifdef __cplusplus
}
#endif

#endif
