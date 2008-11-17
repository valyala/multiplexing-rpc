#ifndef MRPC_PACKET_STREAM_FACTORY_PRIVATE_H
#define MRPC_PACKET_STREAM_FACTORY_PRIVATE_H

#include "private/mrpc_common.h"
#include "private/mrpc_packet_stream.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates the ff_stream wrapper for the mrpc_packet_stream.
 * Always returns correct result.
 */
struct ff_stream *mrpc_packet_stream_factory_create_stream(struct mrpc_packet_stream *packet_stream);

#ifdef __cplusplus
}
#endif

#endif
