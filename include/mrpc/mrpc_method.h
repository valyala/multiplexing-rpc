#ifndef MRPC_METHOD_PUBLIC
#define MRPC_METHOD_PUBLIC

#include "mrpc/mrpc_common.h"
#include "mrpc/mrpc_data.h"
#include "mrpc/mrpc_param.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_method;

typedef void (*mrpc_method_callback)(struct mrpc_data *data, void *service_ctx);
typedef struct mrpc_method *(*mrpc_method_constructor)();

MRPC_API struct mrpc_method *mrpc_method_create_server(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, mrpc_method_callback callback);

MRPC_API struct mrpc_method *mrpc_method_create_client(const mrpc_param_constructor *request_param_constructors,
	const mrpc_param_constructor *response_param_constructors, int *is_key);

MRPC_API void mrpc_method_delete(struct mrpc_method *method);

#ifdef __cplusplus
}
#endif

#endif
