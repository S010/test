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

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
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
erle_encode(const unsigned char *buf, size_t size, unsigned char *outbuf)
{
	const size_t	 endp = size * CHAR_BIT;
	size_t		 readp;
	size_t		 writep = 0;
	size_t		 len;
	unsigned char	 bit;
	size_t		 mask;
	size_t		 prefix;

	for (readp = 0; readp < endp; /* empty */) {
		bit = BIT_AT(readp);
		for (len = 1, ++readp; readp < endp; ++len, ++readp)
			if (!!bit ^ !!BIT_AT(readp))
				break;
		PUT_BIT(bit);
		mask = 1;
		prefix = len;
		while (prefix >>= 1) {
			PUT_BIT(0);
			mask <<= 1;
		}
		while (mask) {
			PUT_BIT(len & mask);
			mask >>= 1;
		}
	}

	return writep;
}

static size_t
erle_decode(const unsigned char *buf, size_t endp, unsigned char *outbuf)
{
	size_t		 readp;
	size_t		 writep = 0;
	unsigned char	 bit;
	size_t		 len;
	size_t		 nbits;

	for (readp = 0; readp < endp; /* empty */) {
		bit = BIT_AT(readp);
		++readp;
		for (nbits = 0; !BIT_AT(readp); ++readp)
			++nbits;
		++readp;
		len = 1;
		while (nbits--) {
			len <<= 1;
			if (BIT_AT(readp))
				len |= 1;
			++readp;
		}
		while (len--)
			PUT_BIT(bit);
	}

	return writep;
}

#undef BIT_AT
#undef PUT_BIT

// Run program in encoding mode.
void
encode(const char *inpath, const char *outpath)
{
	unsigned char	*inbuf;
	size_t		 insize;
	unsigned char	*outbuf;
	size_t		 outsize;
	size_t		 nbits;

	load(inpath, &inbuf, &insize);
	nbits = erle_encode(inbuf, insize, NULL);
	outsize = nbits / 8 + ((nbits % 8) ? 1 : 0);
	outbuf = xmalloc(outsize);

	erle_encode(inbuf, insize, outbuf);

	store(outpath, &nbits, sizeof nbits, O_CREAT | O_TRUNC);
	store(outpath, outbuf, outsize, O_APPEND);
	free(inbuf);
	free(outbuf);
}

// Run program in decoding mode.
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
	nbitsout = erle_decode(inbuf + sizeof(nbitsin), nbitsin, NULL);
	outsize = nbitsout / CHAR_BIT + ((nbitsout % CHAR_BIT) ? 1 : 0);
	outbuf = xmalloc(outsize);

	erle_decode(inbuf + sizeof(nbitsin), nbitsin, outbuf);

	store(outpath, outbuf, outsize, O_CREAT | O_TRUNC);
	free(inbuf);
	free(outbuf);
}
