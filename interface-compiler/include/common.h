#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

void open_file(const char *format, ...);

void close_file();

void dump(const char *format, ...);

void die(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
