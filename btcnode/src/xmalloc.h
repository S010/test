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
		syslog(LOG_CRIT, "failed to allocate %lu * %lu bytes of memory, errno %d", n, size, errno);
		exit(1);
	}
	return ptr;
}

inline void *
xrealloc(void *ptr, size_t size)
{
	void *tmp = realloc(ptr, size);
	if (tmp == NULL) {
		syslog(LOG_CRIT, "failed to reallocate %lu bytes of memory, errno %d", size, errno);
		exit(1);
	}
	return tmp;
}

inline void *
xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		syslog(LOG_CRIT, "failed to allocate %lu bytes of memory, errno %d", size, errno);
		exit(1);
	}
	return ptr;
}

#endif
