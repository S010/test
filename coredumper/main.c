#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif

#define PROGRAM_NAME	"coredumper"
#define DIR_PATH	"/var/coredump"
#define DIR_MODE	0700
#define CORE_PATH	DIR_PATH "/core_%d"
#define CORE_MODE	0600
#define REPORT_PATH	DIR_PATH "/report_%d"
#define REPORT_MODE	0600

static void
fatal(const char *fmt, ...)
{
	va_list	 ap;

	va_start(ap, fmt);
	vsyslog(LOG_CRIT, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void
xsnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list	 ap;
	int	 n;

	va_start(ap, fmt);
	n = vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	if (n < 0)
		fatal("failed to format string, errno=%d", errno);
	if (n > strlen(buf))
		fatal("formatted string truncated");
}

static void
create_dir(void)
{
	struct stat	 st;

	if (stat(DIR_PATH, &st) < 0) {
		if (errno != ENOENT)
			fatal("stat %s, errno=%d", DIR_PATH, errno);
	} else if (S_ISDIR(st.st_mode)) {
		if (chmod(DIR_PATH, DIR_MODE) < 0)
			fatal("chmod %s, errno=%d", DIR_PATH, errno);
		return;
	}

	if (mkdir(DIR_PATH, DIR_MODE) < 0)
		fatal("mkdir %s, errno=%d", DIR_PATH, errno);
}

static int
next_index(void)
{
	struct stat	 st;
	char		 path[PATH_MAX];
	int		 i;

	for (i = 0; i < INT_MAX; ++i) {
		xsnprintf(path, sizeof path, CORE_PATH, i);
		if (stat(path, &st) < 0) {
			if (errno != ENOENT)
				fatal("stat %s, errno=%d", path, errno);
			return i;
		}
	}
	fatal("maximum number of core files reached");

	return -1;
}

static void
create_coredump(int index)
{

	char	 path[PATH_MAX];
	char	 buf[8192];
	ssize_t	 nread;
	int	 fd;

	xsnprintf(path, sizeof path, CORE_PATH, index);

	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, CORE_MODE);
	if (fd < 0)
		fatal("failed to create core file, errno=%d", errno);

	for (;;) {
		nread = read(STDIN_FILENO, buf, sizeof buf);
		if (nread < 0)
			fatal("read, errno=%d", errno);
		if (nread == 0)
			break;
		if (write(fd, buf, nread) != nread)
			fatal("failed to write data to core file, errno=%d", errno);
	}
	close(fd);
}

static ssize_t
read_file(const char *path, char *buf, size_t size)
{
	int	 fd;
	ssize_t	 nread;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	nread = read(fd, buf, size);
	if (nread < 0)
		fatal("failed to read from %s, errno=%d", path, errno);
	close(fd);

	return nread;
}

static void
create_report(int index, pid_t pid)
{
	char			 path[PATH_MAX];
	char			 buf[4096];
	int			 fd;
	ssize_t			 n;
	struct user_regs_struct	 regs = {};
	int			 status;

	if (pid < 0)
		return;

	xsnprintf(path, sizeof path, REPORT_PATH, index);
	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, REPORT_MODE);
	if (fd < 0)
		fatal("failed to create report file, errno=%d", errno);

#define APPEND(...)									\
	do {										\
		char	 tmpbuf[256];							\
		(void) snprintf(tmpbuf, sizeof tmpbuf, __VA_ARGS__);				\
		if (write(fd, tmpbuf, strlen(tmpbuf)) < 0)					\
			fatal("failed to write to report file, errno=%d", errno);	\
	} while (0)

	xsnprintf(path, sizeof path, "/proc/%d/comm", pid);
	n = read_file(path, buf, sizeof(buf) - 1);
	if (n > 0)
		buf[n - 1] = '\0';
	APPEND("comm\t%s\n", buf);

	xsnprintf(path, sizeof path, "/proc/%d/cmdline", pid);
	n = read_file(path, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	for (--n; n >= 0; --n)
		if (buf[n] == '\0')
			buf[n] = ' ';
	APPEND("cmdline\t%s\n", buf);

#if 0
	APPEND("%%\n");

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
		fatal("failed to attach to process, errno=%d", errno);
	// FIXME hangs
	(void) waitpid(pid, &status, WUNTRACED);
	// FIXME returns ESRCH
	if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) < 0)
		fatal("failed to retrieve register values, errno=%d", errno);
	(void) ptrace(PTRACE_DETACH, pid, NULL, NULL);
	APPEND("rip\t0x%llx\n", regs.rip);
	APPEND("rax\t0x%llx\n", regs.rax);
	APPEND("rcx\t0x%llx\n", regs.rcx);
	APPEND("rdx\t0x%llx\n", regs.rdx);
#endif // 0

#undef APPEND

	close(fd);
}

int
main(int argc, char **argv)
{
	int	 i;
	pid_t	 pid = -1;

	openlog(PROGRAM_NAME, 0, LOG_DAEMON);
	(void) atexit(closelog);

	if (argc > 1)
		pid = atoi(argv[1]);

	create_dir();
	i = next_index();
	create_coredump(i);
	create_report(i, pid);

	return EXIT_SUCCESS;
}
