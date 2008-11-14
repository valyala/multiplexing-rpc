#ifndef MRPC_WCHAR_ARRAY_PARAM_PUBLIC
#define MRPC_WCHAR_ARRAY_PARAM_PUBLIC

#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the mrpc_param structure for the mrpc_char_array.
 * Always returns correct result.
 * This function can be used only as mrpc_param_constructor,
 * which must be passed to the mrpc_method_create_server_method() or mrpc_method_create_client_method()
 * in order to create corresponding method.
 */
MRPC_API struct mrpc_param *mrpc_wchar_array_param_create();

#ifdef __cplusplus
}
#endif

#endif
