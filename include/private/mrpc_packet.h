#ifndef MRPC_PACKET_PRIVATE_H
#define MRPC_PACKET_PRIVATE_H

#include "private/mrpc_common.h"
#include "ff/ff_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mrpc_packet_type
{
	MRPC_PACKET_START,
	MRPC_PACKET_MIDDLE,
	MRPC_PACKET_END,
	MRPC_PACKET_SINGLE
};

struct mrpc_packet;

/**
 * Creates a packet.
 * Always returns correct result.
 */
struct mrpc_packet *mrpc_packet_create();

/**
 * Deletes the packet.
 */
void mrpc_packet_delete(struct mrpc_packet *packet);

/**
 * Resets the packet, so it can be used again for either
 * reading data from the stream by using the mrpc_packet_read_from_stream()
 * either writing data into the packet by using the mrpc_packet_write_data().
 */
void mrpc_packet_reset(struct mrpc_packet *packet);

/**
 * Returns the packet's request_id
 */
uint8_t mrpc_packet_get_request_id(struct mrpc_packet *packet);

/**
 * Sets the packet's request_id.
 */
void mrpc_packet_set_request_id(struct mrpc_packet *packet, uint8_t request_id);

/**
 * Returns the packet's type.
 */
enum mrpc_packet_type mrpc_packet_get_type(struct mrpc_packet *packet);

/**
 * Sets the packet's type.
 */
void mrpc_packet_set_type(struct mrpc_packet *packet, enum mrpc_packet_type type);

/**
 * Reads up to len bytes from the packet into the buf.
 * Returns the number of bytes read. It can be in the range 0..len.
 */
int mrpc_packet_read_data(struct mrpc_packet *packet, void *buf, int len);

/**
 * Writes up to len bytes from the buf into the packet.
 * Returns the number of bytes written. It can be in the range 0..len.
 */
int mrpc_packet_write_data(struct mrpc_packet *packet, const void *buf, int len);

/**
 * Read the next packet contents from the stream into the packet.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_packet_read_from_stream(struct mrpc_packet *packet, struct ff_stream *stream);

/**
 * Writes the packet contents to the stream.
 * Returns FF_SUCCESS on success, FF_FAILURE on error.
 */
enum ff_result mrpc_packet_write_to_stream(struct mrpc_packet *packet, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
