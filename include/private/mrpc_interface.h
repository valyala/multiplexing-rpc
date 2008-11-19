#ifndef MRPC_INTERFACE_PRIVATE_H
#define MRPC_INTERFACE_PRIVATE_H

#include "mrpc/mrpc_interface.h"
#include "private/mrpc_method.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the method with the given method_id from the given interface.
 * Returns NULL if the method_id is invalid for the given interface.
 */
const struct mrpc_method *mrpc_interface_get_method(const struct mrpc_interface *interface, uint8_t method_id);

#ifdef __cplusplus
}
#endif

#endif
