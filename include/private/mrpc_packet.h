#ifndef MRPC_PACKET_PRIVATE
#define MRPC_PACKET_PRIVATE

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

struct mrpc_packet *mrpc_packet_create();

void mrpc_packet_delete(struct mrpc_packet *packet);

void mrpc_packet_reset(struct mrpc_packet *packet);

uint8_t mrpc_packet_get_request_id(struct mrpc_packet *packet);

void mrpc_packet_set_request_id(struct mrpc_packet *packet, uint8_t request_id);

enum mrpc_packet_type mrpc_packet_get_type(struct mrpc_packet *packet);

void mrpc_packet_set_type(struct mrpc_packet *packet, enum mrpc_packet_type type);

int mrpc_packet_read_data(struct mrpc_packet *packet, void *buf, int len);

int mrpc_packet_write_data(struct mrpc_packet *packet, const void *buf, int len);

int mrpc_packet_read_from_stream(struct mrpc_packet *packet, struct ff_stream *stream);

int mrpc_packet_write_to_stream(struct mrpc_packet *packet, struct ff_stream *stream);

#ifdef __cplusplus
}
#endif

#endif
