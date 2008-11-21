#include "private/mrpc_common.h"

#include "private/mrpc_int.h"
#include "ff/ff_stream.h"
#include "ff/ff_hash.h"

#define BITS_PER_OCTET 7
#define MAX_UINT_N_OCTETS(n) (((n) + BITS_PER_OCTET - 1) / BITS_PER_OCTET)
#define MAX_UINT64_OCTETS MAX_UINT_N_OCTETS(64)
#define OCTET_MASK ((1 << BITS_PER_OCTET) - 1)
#define CONTINUE_NUMBER_FLAG (1 << BITS_PER_OCTET)

#define MAX_UINT32_VALUE ((1ull << 32) - 1)

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
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot write %d bytes of the serialized uint64 data=%llu to the stream=%p. See previous messages for more info", len, data, stream);
	}
	return result;
}

enum ff_result mrpc_uint64_unserialize(uint64_t *data, struct ff_stream *stream)
{
	uint64_t u_data = 0;
	int bits_len = 0;
	uint8_t octet;
	enum ff_result result = FF_FAILURE;

	do
	{
		if (bits_len >= 64)
		{
			ff_log_debug(L"unexpected length of uint64 value read from the stream=%p. It must be less than or equal to 64", stream);
			result = FF_FAILURE;
			goto end;
		}
		result = ff_stream_read(stream, &octet, 1);
		if (result != FF_SUCCESS)
		{
			ff_log_debug(L"cannot read an octet for bits [%d:%d] from the stream=%p. See previous messages for more info", bits_len, bits_len + 8, stream);
			goto end;
		}
		u_data |= ((uint64_t) (octet & OCTET_MASK)) << bits_len;
		bits_len += BITS_PER_OCTET;
	}
	while ((octet & CONTINUE_NUMBER_FLAG) != 0);

	*data = u_data;
	result = FF_SUCCESS;

end:
	return result;
}

uint32_t mrpc_uint64_get_hash(uint64_t data, uint32_t start_value)
{
	uint32_t hash_value;
	uint32_t tmp[2];

	tmp[0] = (uint32_t) data;
	tmp[1] = (uint32_t) (data >> 32);
	hash_value = ff_hash_uint32(start_value, tmp, 2);
	return hash_value;
}


enum ff_result mrpc_int64_serialize(int64_t data, struct ff_stream *stream)
{
	uint64_t u_data;
	enum ff_result result;

	/* such an encoding allows to minimize the size of serialized signed integers
	 * See ZigZag encoding on the http://code.google.com/apis/protocolbuffers/docs/encoding.html for reference.
	 */
	u_data = (data << 1) ^ (data >> 63);
	result = mrpc_uint64_serialize(u_data, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot serialize uint64 u_data=%llu to the stream=%p. See previous messages for more info", u_data, stream);
	}
	return result;
}

enum ff_result mrpc_int64_unserialize(int64_t *data, struct ff_stream *stream)
{
	uint64_t u_data;
	enum ff_result result;

	result = mrpc_uint64_unserialize(&u_data, stream);
	if (result == FF_SUCCESS)
	{
		/* such an encoding allows to minimize the size of serialized signed integers
		* See ZigZag encoding on the http://code.google.com/apis/protocolbuffers/docs/encoding.html for reference.
		*/
		u_data = (u_data >> 1) ^ (-((int64_t)(u_data & 0x01)));
		*data = (int64_t) u_data;
	}
	else
	{
		ff_log_debug(L"cannot unserialize uint64 from the stream=%p. See previous messages for more info", stream);
	}
	return result;
}

uint32_t mrpc_int64_get_hash(int64_t data, uint32_t start_value)
{
	uint32_t hash_value;
	uint32_t tmp[2];

	tmp[0] = (uint32_t) data;
	tmp[1] = (uint32_t) (data >> 32);
	hash_value = ff_hash_uint32(start_value, tmp, 2);
	return hash_value;
}


enum ff_result mrpc_uint32_serialize(uint32_t data, struct ff_stream *stream)
{
	enum ff_result result;

	result = mrpc_uint64_serialize((uint64_t) data, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot serialize uint32 data=%lu to the stream=%p. See previous messages for more info", data, stream);
	}
	return result;
}

enum ff_result mrpc_uint32_unserialize(uint32_t *data, struct ff_stream *stream)
{
	enum ff_result result;
	uint64_t u_data;

	result = mrpc_uint64_unserialize(&u_data, stream);
	if (result == FF_SUCCESS)
	{
		if (u_data <= MAX_UINT32_VALUE)
		{
			*data = (uint32_t) u_data;
		}
		else
		{
			ff_log_debug(L"value u_data=%llu read from the stream=%p cannot exceed the %llu", u_data, stream, (uint64_t) MAX_UINT32_VALUE);
			result = FF_FAILURE;
		}
	}
	else
	{
		ff_log_debug(L"cannot unserialize uint64 data from the stream=%p. See previous messages for more info", stream);
	}
	return result;
}

uint32_t mrpc_uint32_get_hash(uint32_t data, uint32_t start_value)
{
	uint32_t hash_value;

	hash_value = ff_hash_uint32(start_value, &data, 1);
	return hash_value;
}


enum ff_result mrpc_int32_serialize(int32_t data, struct ff_stream *stream)
{
	uint32_t u_data;
	enum ff_result result;

	/* such an encoding allows to minimize the size of serialized signed integers
	 * See ZigZag encoding on the http://code.google.com/apis/protocolbuffers/docs/encoding.html for reference.
	 */
	u_data = (data << 1) ^ (data >> 31);
	result = mrpc_uint32_serialize(u_data, stream);
	if (result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot serialize uint64 u_data=%lu to the stream=%p. See previous messages for more info", u_data, stream);
	}
	return result;
}

enum ff_result mrpc_int32_unserialize(int32_t *data, struct ff_stream *stream)
{
	uint32_t u_data;
	enum ff_result result;

	result = mrpc_uint32_unserialize(&u_data, stream);
	if (result == FF_SUCCESS)
	{
		/* such an encoding allows to minimize the size of serialized signed integers
		* See ZigZag encoding on the http://code.google.com/apis/protocolbuffers/docs/encoding.html for reference.
		*/
		u_data = (u_data >> 1) ^ (-(int32_t)((u_data & 0x01)));
		*data = (int32_t) u_data;
	}
	else
	{
		ff_log_debug(L"cannot unserialize uint32 data from the stream=%p. See previous messages for more info", stream);
	}
	return result;
}

uint32_t mrpc_int32_get_hash(int32_t data, uint32_t start_value)
{
	uint32_t hash_value;

	hash_value = ff_hash_uint32(start_value, (uint32_t *) &data, 1);
	return hash_value;
}
