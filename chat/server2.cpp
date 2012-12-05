#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <err.h>
#include <poll.h>

#include <json.hpp>

#define DEFAULT_PORT    1234

static void     server(int);

int main(int argc, char **argv) {
    extern char    *optarg;
    int         c, port = DEFAULT_PORT;

    while ((c = getopt(argc, argv, "hdp:")) != -1) {
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            usage(*argv);
            return 1;
        }
    }

    server(port);

    return 0;
}

class recvbuf {
    public:
        recvbuf() :
            buf(nullptr),
            bufsize(0)
        {
        }

        void read(int s) {
        }

    private:
        char *buf;
        size_t bufsize;
};

static void server(int port) {
    int                  s, conn, n_events;
    struct sockaddr_in   sa;
    char buf[8192];
    const size_t bufsize = sizeof(buf);
    ssize_t n_bytes;
    std::map<int, std::ostringstream *> ostrs;
    std::vector<struct pollfd> pollfds;

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

    pollfds.push_back({ s, POLLIN });

    for ( ;; ) {
        dbgprintf("awaiting event...\n");

        n_events = poll(pollfds.data(), pollfds.size(). 0);

        if (n_events == -1)
            err(1, "poll");

        if (pollfds[0].revents & POLLIN) {
            dbgprintf("received connection\n");
            conn = accept(s, NULL, NULL);
            if (conn == -1) {
                warn("accept");
                continue;
            }
        }

        pollfds.push_back({ conn, POLLIN });
        ostrs.insert(std::make_pair(conn, new std::ostringstream()));

        for (auto i = pollfds.begin(); i != pollfds.end(); ++i) {
            if (i->revents & POLLIN) {
                dbgprintf("fd %d has POLLIN\n", i->fd);
                n_bytes = read(conn, buf, bufsize);
                if (n_bytes == -1) {
                    warn("read");
                    continue;
                }
                std::ostringstream *ostrp = ostrs[i->fd];
                char *p = (char *) memchr(buf, '\n', n_bytes);
                if (p == NULL) {
                    ostrp->write(buf, n_bytes);
                } else {
                    ostrp->write(buf, p - buf);
                    try {
                        json::value val(json::parse(ostrp->str()));
                        dbgprintf("received json msg on conn fd %d: %s\n", i->fd, val.str().c_str());
                    } catch (const json::parse_error &e) {
                        dbgprintf("received bogus json msg: %s\n", ostrp->str().c_str());
                    }
                    ostrp->str("");
                }
            } else if (i->revents & (POLLERR | POLLHUP | POLLNVAL | POLLRDHUP)) {
                close(i->fd);
                delete ostrs[i->fd];
                ostrs.erase(i->fd);
            }
        }
        std::remove_if(
            pollfds.begin(),
            pollfds.end(),
            [](const pollfd &pfd) {
                return pfd.revents & (POLLERR | POLLHUP | POLLNVAL | POLLRDHUP);
            }
        );

        dbgprintf("echoed the hostname\n");

        close(conn);
    }
}

static int dbgprintf(const char *fmt, ...) {
    va_list     ap;
    int     n;

    if (!dflag)
        return 0;

    va_start(ap, fmt);
    n = vprintf(fmt, ap);
    va_end(ap);

    return n;
}

static void daemonize(void) {
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
