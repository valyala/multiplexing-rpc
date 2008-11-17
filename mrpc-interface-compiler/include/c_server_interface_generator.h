#ifndef C_SERVER_INTERFACE_GENERATOR_H
#define C_SERVER_INTERFACE_GENERATOR_H

#include "common.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif

void c_server_interface_generator_generate(struct interface *interface);

#ifdef __cplusplus
}
#endif

#endif
