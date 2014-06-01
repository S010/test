#ifndef MAIN_H
#define MAIN_H

#include <sys/types.h>

void	*xmalloc(size_t);
ssize_t	 xwrite(int, const void *, size_t);
void	 load(const char *, unsigned char **, size_t *);
void	 store(const char *, const void *, size_t, int);

#endif
