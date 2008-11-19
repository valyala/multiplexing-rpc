#ifndef C_CLIENT_GENERATOR_H
#define C_CLIENT_GENERATOR_H

#include "common.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif

void c_client_generator_generate(const struct interface *interface);

#ifdef __cplusplus
}
#endif

#endif
