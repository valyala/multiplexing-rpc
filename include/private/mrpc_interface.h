#ifndef MRPC_INTERFACE_PRIVATE
#define MRPC_INTERFACE_PRIVATE

#include "mrpc/mrpc_interface.h"
#include "private/mrpc_method.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_method *mrpc_interface_get_method(struct mrpc_interface *interface, uint8_t method_id);

#ifdef __cplusplus
}
#endif

#endif
