#include "private/mrpc_common.h"

#include "private/mrpc_int_serialization.h"
#include "ff/ff_stream.h"

#define BITS_PER_OCTET 7
#define MAX_UINT_N_OCTETS(n) (((n) + BITS_PER_OCTET - 1) / BITS_PER_OCTET)
#define MAX_UINT64_OCTETS MAX_UINT_N_OCTETS(64)
#define OCTET_MASK ((1 << BITS_PER_OCTET) - 1)
#define CONTINUE_NUMBER_FLAG (1 << BITS_PER_OCTET)

#define MAX_UINT32_VALUE (~0ull)

enum ff_result mrpc_uint64_serialize(uint64_t data, struct ff_stream *stream)
{
	int len;
	uint8_t buf[MAX_UINT64_OCTETS];
	enum ff_result result;

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

	result = ff_stream_write(stream, buf, len);
	return result;
}

enum ff_result mrpc_uint64_unserialize(uint64_t *data, struct ff_stream *stream)
{
	int len = 0;
	uint64_t u_data = 0;
	uint8_t octet;
	enum ff_result result = FF_FAILURE;

	do
	{
		if (len >= MAX_UINT64_OCTETS)
		{
			result = FF_FAILURE;
			goto end;
		}
		result = ff_stream_read(stream, &octet, 1);
		if (result != FF_SUCCESS)
		{
			goto end;
		}
		len++;
		u_data <<= BITS_PER_OCTET;
		u_data |= (octet & OCTET_MASK);
	}
	while ((octet & CONTINUE_NUMBER_FLAG) != 0);

	*data = u_data;
	result = FF_SUCCESS;

end:
	return result;
}

enum ff_result mrpc_int64_serialize(int64_t data, struct ff_stream *stream)
{
	uint64_t u_data;
	enum ff_result result;

	u_data = (data << 1) ^ (data >> 63);
	result = uint64_serialize(u_data, stream);

	return result;
}

enum ff_result mrpc_int64_unserialize(int64_t *data, struct ff_stream *stream)
{
	enum ff_result result;

	result = uint64_unserialize(&u_data, stream);
	if (result == FF_SUCCESS)
	{
		uint64_t u_data;

		u_data = (u_data >> 1) ^ (-(u_data & 0x01));
		*data = (int64_t) u_data;
	}

	return result;
}

enum ff_result mrpc_uint32_serialize(uint32_t data, struct ff_stream *stream)
{
	enum ff_result result;

	result = mrpc_uint64_serialize((uint64_t) data, stream);
	return result;
}

enum ff_result mrpc_uint32_unserialize(uint32_t *data, struct ff_stream *stream)
{
	enum ff_result result;
	uint64_t u64_data;

	result = mrpc_uint64_unserialize(&u64_data, stream);
	if (result == FF_SUCCESS)
	{
		if (u_data < MAX_UINT32_VALUE)
		{
			*data = (uint32_t) u_data;
		}
		else
		{
			result = FF_FAILURE;
		}
	}
	return result;
}

enum ff_result mrpc_int32_serialize(int32_t data, struct ff_stream *stream)
{
	uint32_t u_data;
	enum ff_result result;

	u_data = (data << 1) ^ (data >> 31);
	result = uint32_serialize(u_data, stream);

	return result;
}

enum ff_result mrpc_int32_unserialize(int32_t *data, struct ff_stream *stream)
{
	enum ff_result result;

	result = uint32_unserialize(&u_data, stream);
	if (result == FF_SUCCESS)
	{
		uint32_t u_data;

		u_data = (u_data >> 1) ^ (-(u_data & 0x01));
		*data = (int32_t) u_data;
	}

	return result;
}
