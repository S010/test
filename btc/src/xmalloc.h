/*
 * Copyright (c) 2017 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
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

#ifndef XMALLOC_H
#define XMALLOC_H

#include <syslog.h>
#include <stdlib.h>
#include <errno.h>

inline void *
xcalloc(size_t n, size_t size)
{
	void *ptr = calloc(n, size);
	if (ptr == NULL) {
		syslog(LOG_CRIT, "failed to allocate %zu * %zu bytes of memory, errno %d", n, size, errno);
		exit(1);
	}
	return ptr;
}

inline void *
xrealloc(void *ptr, size_t size)
{
	void *tmp = realloc(ptr, size);
	if (tmp == NULL) {
		syslog(LOG_CRIT, "failed to reallocate %zu bytes of memory, errno %d", size, errno);
		exit(1);
	}
	return tmp;
}

inline void *
xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		syslog(LOG_CRIT, "failed to allocate %zu bytes of memory, errno %d", size, errno);
		exit(1);
	}
	return ptr;
}

#endif
