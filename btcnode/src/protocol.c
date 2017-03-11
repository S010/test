#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "protocol.h"

static int
calc_msg_checksum(const void *in, size_t in_size, uint8_t *out/*[4]*/)
{
	uint8_t hash[2][SHA256_DIGEST_LENGTH];

	for (int i = 0; i < 2; ++i) {
		SHA256(in, in_size, hash[i]);
		in = hash[i];
		in_size = SHA256_DIGEST_LENGTH;
	}
	memcpy(out, hash[1], 4);

	return 0;
}

static size_t
calc_version_msg_len(const struct version_msg *msg)
{
	size_t len;

	// FIXME do sizeof of actual members
	len =
	    sizeof(int32_t) + // version
	    sizeof(uint64_t) + // services
	    sizeof(uint64_t) + // timestamp

	    sizeof(uint64_t) + // recv_services
	    16 + // recv_ip
	    sizeof(uint16_t) + // recv_port

	    sizeof(uint64_t) + // trans_services
	    16 + // trans_ip
	    sizeof(uint16_t) + // trans_port

	    sizeof(uint64_t) + // nonce

	    1 + msg->user_agent_len + 

	    sizeof(int32_t) + // start_height

	    1; // relay

	return len;
}

size_t
marshal_version_msg(const struct version_msg *msg, uint8_t *out)
{
	size_t off = 0;
	off += marshal(msg->version, out + off);
	off += marshal(msg->services, out + off);
	off += marshal(msg->timestamp, out + off);
	off += marshal(msg->recv_services, out + off);
	off += marshal(&msg->recv_ip, out + off);
	off += marshal(htons(msg->recv_port), out + off);
	off += marshal(msg->trans_services, out + off);
	off += marshal(&msg->trans_ip, out + off);
	off += marshal(htons(msg->trans_port), out + off);
	off += marshal(msg->nonce, out + off);
	off += marshal(msg->user_agent_len, out + off);
	off += marshal_bytes(msg->user_agent, msg->user_agent_len, out + off);
	off += marshal(msg->start_height, out + off);
	off += marshal(msg->relay, out + off);
	return off;
}

size_t
marshal_msg_hdr(const struct msg_hdr *hdr, uint8_t *out)
{
	size_t off = 0;
	off += marshal_bytes(hdr->start, sizeof(hdr->start), out + off);
	off += marshal_bytes(hdr->command, sizeof(hdr->command), out + off);
	off += marshal(hdr->payload_size, out + off);
	off += marshal_bytes(hdr->checksum, sizeof(hdr->checksum), out + off);
	return off;
}

size_t
marshal_in6_addr(const struct in6_addr *addr, uint8_t *out)
{
	for (int i = 15; i >= 0; --i) {
		*out++ = ((const uint8_t *)addr)[i];
	}
	return 16;
}

static int
fill_msg_hdr(
    const uint8_t *start,
    const char *command,
    const void *payload,
    size_t payload_size,
    struct msg_hdr *hdr)
{
	memcpy(hdr->start, start, sizeof(hdr->start));
	memset(hdr->command, 0, sizeof(hdr->command));
	strcpy(hdr->command, command);
	hdr->payload_size = payload_size;
	calc_msg_checksum(payload, payload_size, hdr->checksum);

	return 0;
}

int
write_version_msg(const struct version_msg *msg, int fd)
{
	struct msg_hdr hdr;
	size_t msg_len = calc_version_msg_len(msg);
	uint8_t buf[MSG_HDR_LEN + msg_len];

	marshal(msg, buf + MSG_HDR_LEN);
	fill_msg_hdr(MAINNET_START, "version", buf + MSG_HDR_LEN, msg_len, &hdr);
	marshal(&hdr, buf);

	int n = write(fd, buf, sizeof(buf));
	if (n < 0)
		return n;
	else if ((size_t)n != sizeof(buf))
		return -1;
	return 0;
}

static int
read_msg_hdr(int fd, struct msg_hdr *hdr)
{
	ssize_t n;
	uint8_t buf[MSG_HDR_LEN];
	uint8_t *in = buf;

	n = read(fd, buf, sizeof(buf));
	if (n == -1) {
		syslog(LOG_ERR, "%s(): read failed, errno %d", __func__, errno);
		return n;
	} else if (n != sizeof(buf)) {
		syslog(LOG_ERR, "%s(): short read of %d bytes", __func__, (int)n);
		return -1;
	}

	in += unmarshal_bytes(in, sizeof(hdr->start), hdr->start);
	in += unmarshal_bytes(in, sizeof(hdr->command), hdr->command);
	in += unmarshal_uint32(in, &hdr->payload_size);
	in += unmarshal_bytes(in, sizeof(hdr->checksum), hdr->checksum);

	return 0;
}

int
read_version_msg(int fd, struct version_msg *msg)
{
	ssize_t n;
	struct msg_hdr hdr;

	n = read_msg_hdr(fd, &hdr);
	if (n != 0) {
		syslog(LOG_ERR, "%s(): read_msg_hdr failed", __func__);
		return n;
	}

	if (hdr.payload_size < MIN_VERSION_MSG_LEN) {
		syslog(LOG_ERR, "%s(): payload_size is too small at %u bytes", __func__, (unsigned)hdr.payload_size);
		return 1;
	}
	if (hdr.payload_size > 256) {
		syslog(LOG_ERR, "%s(): payload_size too great at %u bytes", __func__, (unsigned)hdr.payload_size);
		return 2;
	}

	uint8_t buf[hdr.payload_size];
	uint8_t checksum[4];
	uint8_t *in = buf;

	n = read(fd, buf, hdr.payload_size);
	if (n == -1) {
		syslog(LOG_ERR, "%s(): read failed, errno %d", __func__, errno);
		return -1;
	}
	if (n != hdr.payload_size) {
		syslog(LOG_ERR, "%s(): short read of %u bytes", __func__, (unsigned)n);
		return -1;
	}
	in += unmarshal(in, (uint32_t *)&msg->version);
	in += unmarshal(in, &msg->services);
	in += unmarshal(in, &msg->timestamp);
	in += unmarshal(in, &msg->recv_services);
	in += unmarshal_bytes(in, sizeof(msg->recv_ip), &msg->recv_ip);
	in += unmarshal(in, &msg->recv_port);
	in += unmarshal(in, &msg->trans_services);
	in += unmarshal_bytes(in, sizeof(msg->trans_ip), &msg->trans_ip);
	in += unmarshal(in, &msg->trans_port);
	in += unmarshal(in, &msg->nonce);
	in += unmarshal(in, &msg->user_agent_len);
	if (msg->user_agent_len >= 0xfd) {
		syslog(LOG_ERR, "%s(): user agent string is longer than we can handle", __func__);
		return 3;
	}
	if ((hdr.payload_size - (in - buf) - 4 - 1) != msg->user_agent_len) {
		syslog(LOG_ERR, "%s(): the user agent length is wrong at %u bytes", __func__, msg->user_agent_len);
		return 4;
	}
	in += unmarshal_varstr(in, msg->user_agent_len, &msg->user_agent);
	in += unmarshal(in, (uint32_t *)&msg->start_height);
	in += unmarshal(in, &msg->relay);

	calc_msg_checksum(buf, hdr.payload_size, checksum);

	if (memcmp(checksum, hdr.checksum, 4) != 0) {
		syslog(LOG_ERR, "%s(): the payload chekcsum appears to be wrong", __func__);
		return 5;
	}

	return 0;
}
