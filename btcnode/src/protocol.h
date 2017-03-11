#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <arpa/inet.h>
#include <string.h>

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
	int32_t version;
	uint64_t services;
	uint64_t timestamp;

	uint64_t recv_services;
	uint8_t recv_ip[16]; // FIXME Make this and trans_ip in6_addr
	uint16_t recv_port;

	uint64_t trans_services;
	uint8_t trans_ip[16];
	uint16_t trans_port;

	uint64_t nonce;

	uint8_t user_agent_len;
	const char *user_agent;

	int32_t start_height;

	uint8_t relay;
};

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

#define marshal_bytes(in, size, out) \
	marshal_bytes_any((const uint8_t *)in, size, out)

inline size_t
marshal_bytes_any(const uint8_t *in, size_t in_size, uint8_t *out)
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

int write_version_msg(const struct version_msg *msg, int fd);

#endif
