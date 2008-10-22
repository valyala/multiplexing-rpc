#ifndef MRPC_METHOD_PUBLIC
#define MRPC_METHOD_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_method;

typedef struct mrpc_method *(*mrpc_method_constructor)();

#ifdef __cplusplus
}
#endif

#endif
