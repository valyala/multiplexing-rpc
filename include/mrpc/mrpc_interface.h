#ifndef MRPC_INTERFACE_PUBLIC
#define MRPC_INTERFACE_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_method.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_interface;

/**
 * Creates a mrpc_interface.
 * method_constructors must be a NULL-terminated array of method constructors, which implement interface's methods.
 * method_constructors must contain at least one non-NULL element.
 * Always returns correct result.
 */
MRPC_API struct mrpc_interface *mrpc_interface_create(const mrpc_method_constructor *method_constructors);

/**
 * Deletes the given interface.
 */
MRPC_API void mrpc_interface_delete(struct mrpc_interface *interface);

#ifdef __cplusplus
}
#endif

#endif
