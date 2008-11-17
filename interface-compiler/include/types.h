#ifndef TYPES_H
#define TYPES_H

#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif

enum param_type
{
	PARAM_UINT32,
	PARAM_INT32,
	PARAM_UINT64,
	PARAM_INT64,
	PARAM_CHAR_ARRAY,
	PARAM_WCHAR_ARRAY,
	PARAM_BLOB,
};

struct param
{
	enum param_type type;
	const char *name;
};

struct param_list
{
	struct param *param;
	struct param_list *next;
};

struct method
{
	struct param_list *request_params;
	struct param_list *response_params;
	const char *name;
};

struct method_list
{
	struct method *method;
	struct method_list *next;
};

struct interface
{
	struct method_list *methods;
	const char *name;
};

#ifdef __cplusplus
}
#endif

#endif
