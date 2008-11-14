#ifndef MRPC_PARAM_PUBLIC
#define MRPC_PARAM_PUBLIC

#include "mrpc/mrpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_param;

typedef struct mrpc_param *(*mrpc_param_constructor)();

#ifdef __cplusplus
}
#endif

#endif
