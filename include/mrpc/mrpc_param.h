#ifndef MRPC_PARAM_PUBLIC_H
#define MRPC_PARAM_PUBLIC_H

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
