#ifndef MRPC_CHAR_ARRAY_PARAM_PUBLIC
#define MRPC_CHAR_ARRAY_PARAM_PUBLIC

#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the mrpc_param structure for the mrpc_char_array.
 * Always returns correct result.
 */
MRPC_API struct mrpc_param *mrpc_char_array_param_create();

#ifdef __cplusplus
}
#endif

#endif
