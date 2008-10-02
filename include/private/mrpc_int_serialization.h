#ifndef MRPC_INT_SERIALIZATION_PRIVATE
#define MRPC_INT_SERIALIZATION_PRIVATE

#include "private/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the 32bit unsigned integer into the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_uint32_serialize(uint32_t data, struct ff_stream *stream);

/**
 * Unserializes the 32bit unsigned integer from the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_uint32_unserialize(uint32_t *data, struct ff_stream *stream);

/**
 * Serializes the 64bit unsigned integer into the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_uint64_serialize(uint64_t data, struct ff_stream *stream);

/**
 * Unserializes the 64bit unsigned integer from the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_uint64_unserialize(uint64_t *data, struct ff_stream *stream);

/**
 * Serializes 32bit signed integer into the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_int32_serialize(int32_t data, struct ff_stream *stream);

/**
 * Unserializes 32bit signed integer from the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_int32_unserialize(int32_t *data, struct ff_stream *stream);

/**
 * Serializes 64bit signed integer into the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_int64_serialize(int64_t data, struct ff_stream *stream);

/**
 * Unserializes 64bit signed integer from the stream.
 * Returns 1 on success, 0 on error.
 */
int mrpc_int64_unserialize(int64_t *data, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
