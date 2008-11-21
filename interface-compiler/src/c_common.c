#include "common.h"
#include "c_common.h"

#include "types.h"

const char *c_get_param_code_type(const struct param *param)
{
	switch (param->type)
	{
		case PARAM_UINT32: return "uint32_t ";
		case PARAM_INT32: return "int32_t ";
		case PARAM_UINT64: return "uint64_t ";
		case PARAM_INT64: return "int64_t ";
		case PARAM_CHAR_ARRAY: return "struct mrpc_char_array *";
		case PARAM_WCHAR_ARRAY: return "struct mrpc_wchar_array *";
		case PARAM_BLOB: return "struct mrpc_blob *";
		default: die("unknown_type for the parameter [%s]", param->name);
	}
	return NULL;
}

const char *c_get_param_type(const struct param *param)
{
	switch (param->type)
	{
		case PARAM_UINT32: return "uint32";
		case PARAM_INT32: return "int32";
		case PARAM_UINT64: return "uint64";
		case PARAM_INT64: return "int64";
		case PARAM_CHAR_ARRAY: return "char_array";
		case PARAM_WCHAR_ARRAY: return "wchar_array";
		case PARAM_BLOB: return "blob";
		default: die("unknown_type for the parameter [%s]", param->name);
	}
	return NULL;
}

const char *c_get_param_code_constructor(const struct param *param)
{
	switch (param->type)
	{
		case PARAM_UINT32: return "mrpc_uint32_param_create";
		case PARAM_INT32: return "mrpc_int32_param_create";
		case PARAM_UINT64: return "mrpc_uint64_param_create";
		case PARAM_INT64: return "mrpc_int64_param_create";
		case PARAM_CHAR_ARRAY: return "mrpc_char_array_param_create";
		case PARAM_WCHAR_ARRAY: return "mrpc_wchar_array_param_create";
		case PARAM_BLOB: return "mrpc_blob_param_create";
		default: die("unknown_constructor for the parameter [%s]", param->name);
	}
	return NULL;
}

int c_is_param_ptr(const struct param *param)
{
	switch (param->type)
	{
	case PARAM_CHAR_ARRAY:
	case PARAM_WCHAR_ARRAY:
	case PARAM_BLOB:
		return 1;
	default:
		return 0;
	}
}
