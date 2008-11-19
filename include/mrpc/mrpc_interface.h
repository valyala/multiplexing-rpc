#ifndef MRPC_INTERFACE_PUBLIC_H
#define MRPC_INTERFACE_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_method.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_interface;

/**
 * Creates client mrpc_interface.
 * method_descriptions must be a NULL-terminated array of method descriptions, which implement interface's methods.
 * method_descriptions must contain at least one non-NULL element.
 * The method_descriptions must be persistent during the returned interface lifetime.
 * The best way is to put it in static memory using the following template code:
 *   static const mrpc_method_client_description client_method_descriptions[] =
 *   {
 *     method_description1,
 *     method_description2,
 *     NULL,
 *   };
 * Always returns correct result.
 */
MRPC_API const struct mrpc_interface *mrpc_interface_client_create(const struct mrpc_method_client_description **method_descriptions);

/**
 * Creates server mrpc_interface.
 * method_descriptions must be a NULL-terminated array of method descriptions, which implement interface's methods.
 * method_descriptions must contain at least one non-NULL element.
 * The method_descriptions must be persistent during the returned interface lifetime.
 * The best way is to put it in static memory using the following template code:
 *   static const mrpc_method_server_description client_method_descriptions[] =
 *   {
 *     method_description1,
 *     method_description2,
 *     NULL,
 *   };
 * Always returns correct result.
 */
MRPC_API const struct mrpc_interface *mrpc_interface_server_create(const struct mrpc_method_server_description **method_descriptions);

/**
 * Deletes the given interface, which was created by either mrpc_interface_client_create()
 * either mrpc_interface_server_create().
 */
MRPC_API void mrpc_interface_delete(const struct mrpc_interface *interface);

#ifdef __cplusplus
}
#endif

#endif
