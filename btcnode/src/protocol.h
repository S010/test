#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <endian.h>

#define PROTOCOL_VERSION 70014

#define MAINNET_START ((const uint8_t[]){ 0xf9, 0xbe, 0xb4, 0xd9 })
#define MAINNET_PORT 8333

#define NODE_NETWORK 0x01

struct msg_hdr {
	uint8_t start[4];
	char command[12];
	uint32_t payload_size;
	uint8_t checksum[4];
};
#define MSG_HDR_LEN ((size_t)(4 + 12 + sizeof(uint32_t) + 4))

struct version_msg {
	int32_t version; // 4 bytes
	uint64_t services; // 8
	uint64_t timestamp; // 8

	uint64_t recv_services; // 8
	struct in6_addr recv_ip; // 16
	uint16_t recv_port; // 2

	uint64_t trans_services; // 8
	struct in6_addr trans_ip; // 16
	uint16_t trans_port; // 2

	uint64_t nonce; // 8

	uint8_t user_agent_len; // 1+
	char user_agent[0xfd - 1]; // ???

	int32_t start_height; // 4

	uint8_t relay; // 1
};
#define MIN_VERSION_MSG_LEN 86

inline size_t
marshal_uint8(uint8_t i, uint8_t *out)
{
	*out++ = i;
	return 1;
}

inline size_t
marshal_uint16(uint16_t i, uint8_t *out)
{
	*out++ = i & 0xff;
	*out++ = i >> 8;
	return 2;
}

inline size_t
marshal_uint32(uint32_t i, uint8_t *out)
{
	*out++ = i & 0xff;
	*out++ = (i >> 8) & 0xff;
	*out++ = (i >> 16) & 0xff;
	*out++ = (i >> 24) & 0xff;
	return 4;
}

inline size_t
marshal_uint64(uint64_t i, uint8_t *out)
{
	int n = 8;
	while (n--) {
		*out++ = i & 0xff;
		i >>= 8;
	}
	return 8;
}

inline size_t
marshal_bytes(const void *in, size_t in_size, void *out)
{
	memcpy(out, in, in_size);
	return in_size;
}

size_t marshal_version_msg(const struct version_msg *msg, uint8_t *out);
size_t marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out);
size_t marshal_in6_addr(const struct in6_addr *addr, uint8_t *out);

#define marshal(x, y) \
	_Generic((x), \
		struct msg_hdr *: marshal_msg_hdr, \
		const struct msg_hdr *: marshal_msg_hdr, \
		struct version_msg *: marshal_version_msg, \
		const struct version_msg *: marshal_version_msg, \
		struct in6_addr *: marshal_in6_addr, \
		const struct in6_addr *: marshal_in6_addr, \
		int32_t: marshal_uint32, \
		uint32_t: marshal_uint32, \
		int16_t: marshal_uint16, \
		uint16_t: marshal_uint16, \
		int64_t: marshal_uint64, \
		uint64_t: marshal_uint64, \
		int8_t: marshal_uint8, \
		uint8_t: marshal_uint8 \
	)((x), (y))

inline size_t
unmarshal_bytes(const void *in, size_t len, void *out)
{
	memcpy(out, in, len);
	return len;
}

inline size_t
unmarshal_uint32(const uint8_t *in, uint32_t *out)
{
	memcpy(out, in, 4);
	*out = le32toh(*out);
	return 4;
}

inline size_t
unmarshal_uint64(const uint8_t *in, uint64_t *out)
{
	memcpy(out, in, 8);
	*out = le64toh(*out);
	return 8;
}

inline size_t
unmarshal_uint16(const uint8_t *in, uint16_t *out)
{
	memcpy(out, in, 2);
	return 2;
}

inline size_t
unmarshal_uint8(const uint8_t *in, uint8_t *out)
{
	*out = *in++;
	return 1;
}

#define unmarshal(x, y) \
	_Generic((y), \
		int64_t *: unmarshal_uint64, \
		uint64_t *: unmarshal_uint64, \
		int32_t *: unmarshal_uint32, \
		uint32_t *: unmarshal_uint32, \
		int16_t *: unmarshal_uint16, \
		uint16_t *: unmarshal_uint16, \
		uint8_t *: unmarshal_uint8 \
	)((x), (y))

int write_version_msg(const struct version_msg *msg, int fd);
int read_version_msg(int fd, struct version_msg *msg);

#endif
