/*
 * Copyright (c) 2014 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include "main.h"

#define BIT_AT(i) (buf[(i) / CHAR_BIT] & (1 << ((i) % CHAR_BIT)))
#define PUT_BIT(b)										\
	do {											\
		if (outbuf != NULL) {								\
			if (b)									\
				outbuf[writep / CHAR_BIT] |= (1 << (writep % CHAR_BIT));	\
			else									\
				outbuf[writep / CHAR_BIT] &= (~(1 << (writep % CHAR_BIT)));	\
		}										\
		++writep;									\
	} while (0)

static size_t
brle_encode(const unsigned char *buf, size_t size, unsigned char *outbuf)
{
	const size_t	 nbits = size * CHAR_BIT;
	size_t		 readp;
	size_t		 writep = 0;
	size_t		 len;
	size_t		 runlen;
	unsigned char	 bit;

	for (readp = 0; readp < nbits; /*empty*/) {
		bit = BIT_AT(readp);
		len = 1;
		while (readp + len < nbits) {
			if ((bit && BIT_AT(readp + len)) || (!bit && !BIT_AT(readp + len)))
				++len;
			else
				break;
		}
		readp += len;
		do {
			PUT_BIT(bit);
			--len;
			runlen = ~0;
			while (runlen > len)
				runlen >>= 1;
			len -= runlen;
			while (runlen > 0) {
				PUT_BIT(1);
				runlen >>= 1;
			}
			PUT_BIT(0);
		} while (len > 0);
	}

	return writep;

}

static size_t
brle_decode(const unsigned char *buf, size_t nbits, unsigned char *outbuf)
{
	size_t		 readp;
	size_t		 writep = 0;
	unsigned char	 bit;
	size_t		 runlen;

	for (readp = 0; readp < nbits; ++readp) {
		bit = BIT_AT(readp);
		++readp;
		for (runlen = 0; BIT_AT(readp); ++readp) {
			runlen <<= 1;
			runlen |= 1;
		}
		PUT_BIT(bit);
		while (runlen--)
			PUT_BIT(bit);
	}

	return writep;
}

#undef BIT_AT
#undef PUT_BIT

void
encode(const char *inpath, const char *outpath)
{
	unsigned char	*inbuf;
	size_t		 insize;
	unsigned char	*outbuf;
	size_t		 outsize;
	size_t		 nbits;

	load(inpath, &inbuf, &insize);
	nbits = brle_encode(inbuf, insize, NULL);
	outsize = nbits / 8 + ((nbits % 8) ? 1 : 0);
	outbuf = xmalloc(outsize);
	brle_encode(inbuf, insize, outbuf);
	store(outpath, &nbits, sizeof nbits, O_CREAT | O_TRUNC);
	store(outpath, outbuf, outsize, O_APPEND);
	free(inbuf);
	free(outbuf);
}

void
decode(const char *inpath, const char *outpath)
{
	unsigned char	*inbuf;
	size_t		 insize;
	unsigned char	*outbuf;
	size_t		 outsize;
	size_t		 nbitsin;
	size_t		 nbitsout;
	size_t		 i;

	load(inpath, &inbuf, &insize);
	for (i = 0, nbitsin = 0; i < sizeof(nbitsin); ++i)
		nbitsin |= inbuf[i] << (CHAR_BIT * i);
	nbitsout = brle_decode(inbuf + sizeof(nbitsin), nbitsin, NULL);
	outsize = nbitsout / CHAR_BIT + ((nbitsout % CHAR_BIT) ? 1 : 0);
	outbuf = xmalloc(outsize);
	brle_decode(inbuf + sizeof(nbitsin), nbitsin, outbuf);
	store(outpath, outbuf, outsize, O_CREAT | O_TRUNC);
	free(inbuf);
	free(outbuf);
}
