#ifndef MRPC_PARAM_VTABLE_PUBLIC
#define MRPC_PARAM_VTABLE_PUBLIC

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_param_vtable
{
	void *(*create)();
	void (*delete)(void *ctx);
	int (*read_from_stream)(void *ctx, struct ff_stream *stream);
	int (*write_to_stream)(const void *ctx, struct ff_stream *stream);
	void (*get_value)(const void *ctx, void **value);
	void (*set_value)(void *ctx, const void *value);
	uint32_t (*get_hash)(void *ctx, uint32_t start_value);
};

#ifdef __cplusplus
}
#endif

#endif
