/*
 * echohostnamed
 *
 * Listen on the specified TCP port, whenever someone connects -- echo
 * our hostname and immediately close the connection.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <err.h>

#include <json.hpp>

#define DEFAULT_PORT    1234

static void     usage(const char *);
static void     echohostnamed(int);
static void     daemonize(void);
static int     dbgprintf(const char *, ...);

int         dflag;

int
main(int argc, char **argv)
{
    extern char    *optarg;
    int         c, port = DEFAULT_PORT;

    while ((c = getopt(argc, argv, "hdp:")) != -1) {
        switch (c) {
        case 'h':
            usage(*argv);
            return 0;
            break;
        case 'd':
            ++dflag;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            usage(*argv);
            return 1;
        }
    }

    echohostnamed(port);

    return 0;
}

static void
usage(const char *argv0)
{
    const char *progname;

    progname = strrchr(argv0, '/');
    if (progname != NULL)
        ++progname;
    else
        progname = argv0;

    printf("usage: %s [-d] [-p <port>]\n", progname);
    printf("  Listens on the specified port (%d by default) and echos whenever someone connects.\n", DEFAULT_PORT);
}

static void
echohostnamed(int port)
{
    int                  s, conn;
    struct sockaddr_in   sa;
    char buf[8192];
    const size_t bufsize = sizeof(buf);
    ssize_t n_bytes;
    std::ostringstream ostr;


    dbgprintf("trying to listen on port %d\n", port);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        err(1, "socket");

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((short) port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1)
        err(1, "bind");
    if (listen(s, 0) == -1)
        err(1, "listen");

    if (!dflag)
        daemonize();

    for ( ;; ) {
        dbgprintf("awaiting connection...\n");

        conn = accept(s, NULL, NULL);
        if (conn == -1) {
            warn("accept");
            continue;
        }

        dbgprintf("received connection\n");

        for ( ;; ) {
            n_bytes = read(conn, buf, bufsize);
            if (n_bytes == -1) {
                warn("read");
                break;
            }
            char *p = memchr(buf, '\n', n_bytes);
            if (p == NULL) {
                // TODO
            }
        }

        dbgprintf("echoed the hostname\n");

        close(conn);
    }
}

static int
dbgprintf(const char *fmt, ...)
{
    va_list     ap;
    int     n;

    if (!dflag)
        return 0;

    va_start(ap, fmt);
    n = vprintf(fmt, ap);
    va_end(ap);

    return n;
}

static void
daemonize(void)
{
    pid_t     pid;

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid = fork();

    if (pid == -1)
        err(1, "fork");
    else if (pid == 0)
        return;
    else
        exit(0);
}
