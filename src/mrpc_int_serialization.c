#include "private/mrpc_common.h"

#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

#define BITS_PER_OCTET 7
#define MAX_UINT_N_OCTETS(n) (((n) + BITS_PER_OCTET - 1) / BITS_PER_OCTET)
#define MAX_UINT32_OCTETS MAX_UINT_N_OCTETS(32)
#define MAX_UINT64_OCTETS MAX_UINT_N_OCTETS(64)
#define OCTET_MASK ((1 << BITS_PER_OCTET) - 1)
#define CONTINUE_NUMBER_FLAG (1 << BITS_PER_OCTET)

int mrpc_uint32_serialize(uint32_t data, struct ff_stream *stream)
{
	int is_success;
	int len = 0;
	uint8_t buf[MAX_UINT32_OCTETS];

	do
	{
		uint8_t octet;

		ff_assert(len < MAX_UINT32_OCTETS);
		octet = (uint8_t) (data & OCTET_MASK);
		data >>= BITS_PER_OCTET;
		octet |= (uint8_t) ((data != 0) ? CONTINUE_NUMBER_FLAG : 0);
		buf[len] = octet;
		len++;
	}
	while (data != 0);

	is_success = ff_stream_write(stream, buf, len);
	return is_success;
}

int mrpc_uint32_unserialize(uint32_t *data, struct ff_stream *stream)
{
	int is_success = 0;
	int len = 0;
	uint32_t u_data = 0;
	uint8_t octet;

	do
	{
		if (len >= MAX_UINT32_OCTETS)
		{
			is_success = 0;
			goto end;
		}
		is_success = ff_stream_read(stream, &octet, 1);
		if (!is_success)
		{
			goto end;
		}
		len++;
		u_data <<= BITS_PER_OCTET;
		u_data |= (octet & OCTET_MASK);
	}
	while ((octet & CONTINUE_NUMBER_FLAG) != 0);

	*data = u_data;
	is_success = 1;

end:
	return is_success;
}

int mrpc_uint64_serialize(uint64_t data, struct ff_stream *stream)
{
	int is_success;
	int len;
	uint8_t buf[MAX_UINT64_OCTETS];

	len = 0;
	do
	{
		uint8_t octet;
		
		ff_assert(len < MAX_UINT64_OCTETS);
		octet = (uint8_t) (data & OCTET_MASK);
		data >>= BITS_PER_OCTET;
		octet |= (uint8_t) ((data != 0) ? CONTINUE_NUMBER_FLAG : 0);
		buf[len] = octet;
		len++;
	}
	while (data != 0);

	is_success = ff_stream_write(stream, buf, len);
	return is_success;
}

int mrpc_uint64_unserialize(uint64_t *data, struct ff_stream *stream)
{
	int is_success = 0;
	int len = 0;
	uint64_t u_data = 0;
	uint8_t octet;

	do
	{
		if (len >= MAX_UINT64_OCTETS)
		{
			is_success = 0;
			goto end;
		}
		is_success = ff_stream_read(stream, &octet, 1);
		if (!is_success)
		{
			goto end;
		}
		len++;
		u_data <<= BITS_PER_OCTET;
		u_data |= (octet & OCTET_MASK);
	}
	while ((octet & CONTINUE_NUMBER_FLAG) != 0);

	*data = u_data;
	is_success = 1;

end:
	return is_success;
}

int mrpc_int32_serialize(int32_t data, struct ff_stream *stream)
{
	int is_success;
	uint32_t u_data;

	u_data = (data << 1) ^ (data >> 31);
	is_success = uint32_serialize(u_data, stream);

	return is_success;
}

int mrpc_int32_unserialize(int32_t *data, struct ff_stream *stream)
{
	int is_success;
	uint32_t u_data;

	is_success = uint32_unserialize(&u_data, stream);
	u_data = (u_data >> 1) ^ (-(u_data & 0x01));
	*data = (int32_t) u_data;

	return is_success;
}

int mrpc_int64_serialize(int64_t data, struct ff_stream *stream)
{
	int is_success;
	uint64_t u_data;

	u_data = (data << 1) ^ (data >> 63);
	is_success = uint64_serialize(u_data, stream);

	return is_success;
}

int mrpc_int64_unserialize(int64_t *data, struct ff_stream *stream)
{
	int is_success;
	uint64_t u_data;

	is_success = uint64_unserialize(&u_data, stream);
	u_data = (u_data >> 1) ^ (-(u_data & 0x01));
	*data = (int64_t) u_data;

	return is_success;
}
