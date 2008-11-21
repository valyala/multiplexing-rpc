#ifndef MRPC_INT_PUBLIC_H
#define MRPC_INT_PUBLIC_H

#include "mrpc/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Serializes the 64bit unsigned integer into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_uint64_serialize(uint64_t data, struct ff_stream *stream);

/**
 * Unserializes the 64bit unsigned integer from the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_uint64_unserialize(uint64_t *data, struct ff_stream *stream);

/**
 * Calculates the hash value for the given 64bit unsigned integer and the given start_value.
 */
MRPC_API uint32_t mrpc_uint64_get_hash(uint64_t data, uint32_t start_value);


/**
 * Serializes 64bit signed integer into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_int64_serialize(int64_t data, struct ff_stream *stream);

/**
 * Unserializes 64bit signed integer from the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_int64_unserialize(int64_t *data, struct ff_stream *stream);

/**
 * Calculates the hash value for the given 64bit signed integer and the given start_value.
 */
MRPC_API uint32_t mrpc_int64_get_hash(int64_t data, uint32_t start_value);


/**
 * Serializes the 32bit unsigned integer into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_uint32_serialize(uint32_t data, struct ff_stream *stream);

/**
 * Unserializes the 32bit unsigned integer from the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_uint32_unserialize(uint32_t *data, struct ff_stream *stream);

/**
 * Calculates the hash value for the given 32bit unsigned integer and the given start_value.
 */
MRPC_API uint32_t mrpc_uint32_get_hash(uint32_t data, uint32_t start_value);


/**
 * Serializes 32bit signed integer into the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_int32_serialize(int32_t data, struct ff_stream *stream);

/**
 * Unserializes 32bit signed integer from the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
MRPC_API enum ff_result mrpc_int32_unserialize(int32_t *data, struct ff_stream *stream);

/**
 * Calculates the hash value for the given 32bit signed integer and the given start_value.
 */
MRPC_API uint32_t mrpc_int32_get_hash(int32_t data, uint32_t start_value);

#ifdef __cplusplus
}
#endif

#endif
