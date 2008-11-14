#ifndef MRPC_PARAM_PRIVATE
#define MRPC_PARAM_PRIVATE

#include "mrpc/mrpc_param.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrpc_param_vtable
{
	void (*delete)(struct mrpc_param *param);
	enum ff_result (*read_from_stream)(struct mrpc_param *param, struct ff_stream *stream);
	enum ff_result (*write_to_stream)(struct mrpc_param *param, struct ff_stream *stream);
	void (*get_value)(struct mrpc_param *param, void **value);
	void (*set_value)(struct mrpc_param *param, const void *value);
	uint32_t (*get_hash)(struct mrpc_param *param, uint32_t start_value);
};

struct mrpc_param *mrpc_param_create(const struct mrpc_param_vtable *vtable, void *ctx);

void mrpc_param_delete(struct mrpc_param *param);

void *mrpc_param_get_ctx(struct mrpc_param *param);

enum ff_result mrpc_param_read_from_stream(struct mrpc_param *param, struct ff_stream *stream);

enum ff_result mrpc_param_write_to_stream(struct mrpc_param *param, struct ff_stream *stream);

void mrpc_param_get_value(struct mrpc_param *param, void **value);

void mrpc_param_set_value(struct mrpc_param *param, const void *value);

uint32_t mrpc_param_get_hash(struct mrpc_param *param, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
