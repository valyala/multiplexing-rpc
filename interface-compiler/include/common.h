#ifndef COMMON_H
#define COMMON_H

#if defined(WIN32)
	#define _CRT_SECURE_NO_WARNINGS
	#define snprintf _snprintf
#endif

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#endif
