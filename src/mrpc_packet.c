#include "private/mrpc_common.h"

#include "private/mrpc_packet.h"
#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

/* this size allows to pack packet header into maximum three bytes:
 * the first byte is the request_id and the second with third bytes
 * contain packet length (14bits) alongside with packet type (2bits).
 * Packet length is variable-length encoded, so the maximum packet size is 2^12 - 1.
 */
#define MAX_PACKET_SIZE ((1 << 12) - 1)

struct mrpc_packet
{
	char *buf;
	int curr_pos;
	int size;
	enum mrpc_packet_type type;
	uint8_t request_id;
};

struct mrpc_packet *mrpc_packet_create()
{
	struct mrpc_packet *packet;

	packet = (struct mrpc_packet *) ff_malloc(sizeof(*packet));
	packet->buf = (char *) ff_calloc(MAX_PACKET_SIZE, sizeof(packet->buf[0]));
	packet->curr_pos = 0;
	packet->size = 0;
	packet->type = MRPC_PACKET_START;
	packet->request_id = 0;

	return packet;
}

void mrpc_packet_delete(struct mrpc_packet *packet)
{
	ff_assert(packet->curr_pos == 0);
	ff_assert(packet->size == 0);

	ff_free(packet->buf);
	ff_free(packet);
}

void mrpc_packet_reset(struct mrpc_packet *packet)
{
	packet->curr_pos = 0;
	packet->size = 0;
	packet->type = MRPC_PACKET_START;
	packet->request_id = 0;
}

uint8_t mrpc_packet_get_request_id(struct mrpc_packet *packet)
{
	return packet->request_id;
}

void mrpc_packet_set_request_id(struct mrpc_packet *packet, uint8_t request_id)
{
	packet->request_id = request_id;
}

enum mrpc_packet_type mrpc_packet_get_type(struct mrpc_packet *packet)
{
	return packet->type;
}

void mrpc_packet_set_type(struct mrpc_packet *packet, enum mrpc_packet_type type)
{
	packet->type = type;
}

int mrpc_packet_read_data(struct mrpc_packet *packet, void *buf, int len)
{
	int bytes_read;
	int bytes_left;

	ff_assert(len >= 0);
	ff_assert(packet->curr_pos >= 0);
	ff_assert(packet->size >= packet->curr_pos);
	ff_assert(packet->size <= MAX_PACKET_SIZE);

	bytes_left = packet->size - packet->curr_pos;
	bytes_read = (len > bytes_left) ? bytes_left : len;
	memcpy(buf, packet->buf + packet->curr_pos, bytes_read);
	packet->curr_pos += bytes_read;

	return bytes_read;
}

int mrpc_packet_write_data(struct mrpc_packet *packet, const void *buf, int len)
{
	int bytes_written;
	int bytes_left;

	ff_assert(len >= 0);
	ff_assert(packet->curr_pos == 0);
	ff_assert(packet->size >= 0);
	ff_assert(packet->size <= MAX_PACKET_SIZE);

	bytes_left = MAX_PACKET_SIZE - packet->size;
	bytes_written = (len > bytes_left) ? bytes_left : len;
	memcpy(packet->buf + packet->size, buf, bytes_written);
	packet->size += bytes_written;

	return bytes_written;
}

enum ff_result mrpc_packet_read_from_stream(struct mrpc_packet *packet, struct ff_stream *stream)
{
	uint32_t tmp;
	enum ff_result result;

	ff_assert(packet->curr_pos == 0);
	ff_assert(packet->size == 0);

	result = ff_stream_read(stream, &packet->request_id, 1);
	if (result != FF_SUCCESS)
	{
		goto end;
	}
	result = mrpc_uint32_unserialize(&tmp, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}
	packet->type = (enum mrpc_packet_type) (tmp & 0x03);
	/* packet type can have only 4 different values, which are mapped to
	 * the enum mrpc_packet_type, so there is no need to check it for correctess
	 */
	packet->size = (int) (tmp >> 2);
	if (packet->size > MAX_PACKET_SIZE)
	{
		result = FF_FAILURE;
		goto end;
	}
	result = ff_stream_read(stream, packet->buf, packet->size);

end:
	return result;
}

enum ff_result mrpc_packet_write_to_stream(struct mrpc_packet *packet, struct ff_stream *stream)
{
	uint32_t tmp;
	enum ff_result result;

	ff_assert(packet->curr_pos == 0);
	ff_assert(packet->size <= MAX_PACKET_SIZE);

	result = ff_stream_write(stream, &packet->request_id, 1);
	if (result != FF_SUCCESS)
	{
		goto end;
	}
	tmp = ((uint32_t) packet->type) | (((uint32_t) packet->size) << 2);
	result = mrpc_uint32_serialize(tmp, stream);
	if (result != FF_SUCCESS)
	{
		goto end;
	}
	result = ff_stream_write(stream, packet->buf, packet->size);

end:
	return result;
}
