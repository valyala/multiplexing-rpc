#ifndef MRPC_INT_PARAM_PUBLIC
#define MRPC_INT_PARAM_PUBLIC

#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the mrpc_param structure for the uint64_t.
 * Always returns correct result.
 */
MRPC_API struct mrpc_param *mrpc_uint64_param_create();

/**
 * Returns the mrpc_param structure for the int64_t.
 * Always returns correct result.
 */
MRPC_API struct mrpc_param *mrpc_int64_param_create();

/**
 * Returns the mrpc_param structure for the uint32_t.
 * Always returns correct result.
 */
MRPC_API struct mrpc_param *mrpc_uint32_param_create();

/**
 * Returns the mrpc_param structure for the int32_t.
 * Always returns correct result.
 */
MRPC_API struct mrpc_param *mrpc_int32_param_create();

#ifdef __cplusplus
}
#endif

#endif
