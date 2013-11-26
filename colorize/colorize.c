#include <sys/types.h>
#include <unistd.h>
#include <regex.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static const struct {
	const char	*name;
	int		 code;
} fgcolors[] = {
	{ "black",  30 },
	{ "red",    31 },
	{ "green",  32 },
	{ "yellow", 33 },
	{ "blue",   34 },
	{ "purple", 35 },
	{ "cyan",   36 },
	{ "white",  37 },
}, bgcolors[] = {
	{ "black",  40 },
	{ "red",    41 },
	{ "green",  42 },
	{ "yellow", 44 },
	{ "blue",   44 },
	{ "purple", 45 },
	{ "cyan",   46 },
	{ "white",  47 },
};

struct rule {
	regex_t		 re;
	int		 fg;
	int		 bg;
	int		 bold;
	struct rule	*next;
};

static void *
xmalloc(size_t size)
{
	void	*p;
	
	p = malloc(size);
	if (p == NULL)
		err(1, "malloc");
	return p;
}

static void *
xrealloc(void *p, size_t size)
{
	p = realloc(p, size);
	if (p == NULL)
		err(1, "realloc");
	return p;
}

static struct rule *
parse(const char *path)
{
	return NULL;
}

static void
strwrite(char **dst, char *src, size_t n)
{
	memcpy(*dst, src, n);
	*dst += n;
}

static void
strappend(char *buf, const char *fmt, ...)
{
	char	 tmp[4096];
	va_list	 ap;
	size_t	 len;

	va_start(ap, fmt);
	vsnprintf(tmp, sizeof tmp, fmt, ap);
	va_end(ap);

	len = strlen(buf);
	strcpy(buf + len, tmp);
}

static char *
escbegin(struct rule *rule)
{
	char	*buf;
	size_t	 bufsize = sizeof "\033[1;31;42m";

	buf = xmalloc(bufsize);
	*buf = '\0';
	strappend(buf, "\033[");
	if (rule->bold > 0)
		strappend(buf, "1%c", (rule->fg || rule->bg) ? ';' : '\0');
	if (rule->fg > 0)
		strappend(buf, "%d%c", rule->fg, rule->bg ? ';' : '\0');
	if (rule->bg > 0)
		strappend(buf, "%d", rule->bg);
	strappend(buf, "m");

	return buf;
}

static char *
escend(struct rule *rule)
{
	char	*buf;
	size_t	 bufsize = sizeof "\033[0m";

	buf = xmalloc(bufsize);
	strcpy(buf, "\033[0m");
	return buf;
}

static void
apply(struct rule *rulep, char **bufp, size_t *bufsizep)
{
	regmatch_t	 match;
	char		*escb;
	char		*esce;
	size_t		 escblen;
	size_t		 escelen;
	char		*buf;
	size_t		 bufsize;
	char		*writep;

	if (regexec(&rulep->re, *bufp, 1, &match, 0) != 0)
		return;

	/* generate the appropriate escape sequences */
	escb = escbegin(rulep);
	esce = escend(rulep);
	escblen = strlen(escb);
	escelen = strlen(esce);

	/* alloc a new buffer, accounting for len of esc sequences */
	bufsize = *bufsizep + escblen + escelen;
	buf = xmalloc(bufsize);
	writep = buf;

	/* copy the string inserting esc sequences in appropriate places */
	strwrite(&writep, *bufp, match.rm_so);
	strwrite(&writep, escb, escblen);
	strwrite(&writep, *bufp + match.rm_so, match.rm_eo - match.rm_so);
	strwrite(&writep, esce, escelen);
	strwrite(&writep, *bufp + match.rm_eo, *bufsizep - match.rm_eo);

	free(escb);
	free(esce);

	free(*bufp);
	*bufp = buf;
	*bufsizep = bufsize;
}

static void
colorize(struct rule *rules, FILE *in, FILE *out)
{
	char		*buf = NULL;
	size_t		 bufsize = 0;
	struct rule	*rp;

	while (getline(&buf, &bufsize, in) != -1) {
		for (rp = rules; rp != NULL; rp = rp->next)
			apply(rp, &buf, &bufsize);
		fwrite(buf, strlen(buf), 1, out);
	}
}

static void
usage(void)
{
	printf("usage: colorize <script>\n");
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;

	struct rule	 rule;

	while ((ch = getopt(argc, argv, "h")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case '?':
			usage();
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	memset(&rule, 0, sizeof rule);
	regcomp(&rule.re, "error[^[:space:]]*", REG_ICASE);
	rule.fg = 31;
	colorize(&rule, stdin, stdout);

	exit(EXIT_SUCCESS);
}
