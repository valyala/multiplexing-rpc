#ifndef MRPC_METHOD_PUBLIC
#define MRPC_METHOD_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_method;

/**
 * This callback must create mrpc_method object using the mrpc_method_create_server_method()
 * or mrpc_method_create_client_method() functions.
 * It must always return correct result.
 * Array of such a callbacks must be passed to the mrpc_interface_create().
 */
typedef struct mrpc_method *(*mrpc_method_constructor)();

#ifdef __cplusplus
}
#endif

#endif
