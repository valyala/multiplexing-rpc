#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

struct interface;

void interface_delete(const struct interface *interface);

#ifdef __cplusplus
}
#endif

#endif
