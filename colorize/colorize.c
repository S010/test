#include <sys/types.h>
#include <unistd.h>
#include <regex.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

static const struct color {
	const char	*name;
	int		 fg;
	int		 bg;
} colors[] = {
	{ "none",   -1, -1 },
	{ "black",  30, 40 },
	{ "red",    31, 41 },
	{ "green",  32, 42 },
	{ "yellow", 33, 44 },
	{ "blue",   34, 44 },
	{ "purple", 35, 45 },
	{ "cyan",   36, 46 },
	{ "white",  37, 47 },
	{ NULL }
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

static char *
strip(char *s)
{
	char	*p;

	while (isspace(*s))
		++s;
	p = s + strlen(s) - 1;
	while (p >= s && isspace(*p))
		*p = '\0';
	return s;
}

static int
parsere(char *buf, struct rule *rule)
{
	char	*end;
	int	 flags = 0;

	buf = strip(buf);
	if (*buf++ != '/')
		return 0;
	end = strrchr(buf, '/');
	if (end == NULL)
		return 0;
	*end = '\0';
	for (++end; *end != '\0'; ++end)
		if (*end == 'i')
			flags |= REG_ICASE;
	if (regcomp(&rule->re, buf, flags) != 0)
		return 0;
	return 1;
}

static int
parsecolors(char *buf, struct rule *rule)
{
	char			*p;
	char			*next;
	const struct color	*cp;

	buf = strip(buf);
	next = buf;
	while ((p = strsep(&next, ",")) != NULL) {
		p = strip(p);
		if (!strcmp(p, "bold")) {
			rule->bold = 1;
			continue;
		}
		for (cp = colors; cp->name != NULL; ++cp)
			if (!strcmp(p, cp->name))
				break;
		if (cp->name == NULL)
			return 0;
		if (rule->fg == 0)
			rule->fg = cp->fg;
		else if (rule->bg == 0)
			rule->bg = cp->bg;
		else /* color specified more than twice */
			return 0;
	}
	if (!rule->fg && !rule->bg && !rule->bold)
		return 0;
	return 1;
}

static int
parseline(char *buf, struct rule **rulep)
{
	struct rule	 rule;
	char		*p;
	char		*delim;

	buf = strip(buf);
	delim = strrchr(buf, ' ');
	if (delim == NULL)
		return 0;
	*delim = '\0';
	memset(&rule, 0, sizeof rule);
	if (!parsere(buf, &rule))
		return 0;
	if (!parsecolors(delim + 1, &rule))
		return 0;
	*rulep = xmalloc(sizeof rule);
	**rulep = rule;
	return 1;
}

static struct rule *
parse(const char *path)
{
	FILE		*fp;
	char		*buf = NULL;
	size_t		 bufsize = 0;
	size_t		 lineno = 0;
	struct rule	*rules = NULL;
	struct rule	*tail = NULL;
	struct rule	*rule;

	fp = fopen(path, "r");
	if (fp == NULL)
		err(1, "fopen: %s", path);

	while (getline(&buf, &bufsize, fp) != -1) {
		++lineno;
		if (!parseline(buf, &rule))
			errx(1, "%s: failed to parse line %d", path, lineno);
		if (rule == NULL)
			continue;
		if (tail == NULL)
			rules = tail = rule;
		else {
			tail->next = rule;
			tail = rule;
		}

	}
	if (!feof(fp))
		err(1, "getline: %s", path);
	free(buf);

	return rules;
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

static void
freerule(struct rule *rule)
{
	if (rule == NULL)
		return;
	freerule(rule->next);
	regfree(&rule->re);
	free(rule);
}

int
main(int argc, char **argv)
{
	int		 ch;
	extern int	 optind;
	struct rule	*rules;

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

	if (argc == 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	rules = parse(*argv);
	colorize(rules, stdin, stdout);

	exit(EXIT_SUCCESS);
}
