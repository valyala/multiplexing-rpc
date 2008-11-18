#ifndef PARSER_H
#define PARSER_H

#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct interface *parse_interface(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
