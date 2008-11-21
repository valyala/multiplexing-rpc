#ifndef C_COMMON_H
#define C_COMMON_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *c_get_param_code_type(const struct param *param);

const char *c_get_param_type(const struct param *param);

const char *c_get_param_code_constructor(const struct param *param);

int c_is_param_ptr(const struct param *param);

#ifdef __cplusplus
}
#endif

#endif
