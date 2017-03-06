#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <openssl/sha.h>

#define MAINNET 8333

struct msg_hdr {
	char magic[4];
	char command[12];
	uint32_t payload_size;
	uint8_t checksum[4];
};

struct version_msg {
	int32_t version;
	uint64_t services;
	uint64_t timestamp;

	uint64_t recv_services;
	uint8_t recv_ip[16];
	uint16_t recv_port;

	uint64_t trans_services;
	uint8_t trans_ip[16];
	uint16_t trans_port;

	uint8_t nonce;

	uint16_t user_agent_len;
	char *user_agent;

	int32_t start_height;

	bool relay;
};

int
calc_msg_hdr_checksum(const void *in, size_t in_size, uint8_t *out/*[4]*/)
{
	SHA256_CTX ctx;
	uint8_t buf[2][SHA_DIGEST_LENGTH];
	int error;

	error = SHA256_Init(&ctx);
	if (error)
		return error;

	for (int i = 0; i < 2; ++i) {
		error = SHA256_Update(&ctx, in, in_size);
		if (error)
			return error;

		error = SHA256_Final(buf[i], &ctx);
		if (error)
			return error;

		in = buf[i];
		in_size = sizeof(buf[i]);
	}

	memcpy(out, buf[1], 4);

	return 0;
}

size_t
calc_version_msg_len(const struct version_msg *msg)
{
	// FIXME
	(void)msg;
	return 0;
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openlog("btcnode", LOG_PID, LOG_LOCAL0);
	syslog(LOG_NOTICE, "started");
	return 0;
}
