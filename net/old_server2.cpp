#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <err.h>
#include <poll.h>

#include <json.hpp>
#include "recvbuf.hpp"

static const int default_port = 1234;
static const int default_poll_timeout = 50; // in milliseconds

static void server(int);

int main(int argc, char **argv) {
    extern char    *optarg;
    int             c, port = default_port;

    while ((c = getopt(argc, argv, "p:")) != -1) {
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            return 1;
        }
    }

    server(port);

    return 0;
}


static void server(int port) {
    int                               s;
    struct sockaddr_in                sa;
    std::vector<struct pollfd>        pfds;
    std::unordered_map<int, recvbuf>  rbufs;

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

    pfds.push_back({ s, POLLIN });

    std::clog << "entering poll loop..." << std::endl;
    for ( ;; ) {
        int npoll = poll(pfds.data(), pfds.size(), default_poll_timeout);

        if (npoll == -1)
            err(1, "poll");
        else if (npoll == 0)
            continue;

        std::clog << npoll << " fds are ready" << std::endl;

        for (auto i = pfds.begin() + 1; i != pfds.end(); ++i) {
            if (i->revents & (POLLIN | POLLPRI)) {
                recvbuf &rbuf = rbufs[i->fd];
                rbuf.read(i->fd);
                if (rbuf)
                    std::cout << "got message: " << rbuf.str() << std::endl;
            } else if (i->revents != 0) {
                close(i->fd);
                rbufs.erase(i->fd);
            }
        }
        pfds.erase(
            std::remove_if(
                pfds.begin() + 1,
                pfds.end(),
                [](pollfd &p) {
                    return p.revents & (POLLERR | POLLHUP | POLLNVAL);
                }
            ),
            pfds.end()
        );

        if (pfds[0].revents & POLLIN) {
            std::clog << "accepting a new connection" << std::endl;

            int conn = accept(pfds[0].fd, NULL, NULL);
            if (conn == -1) {
                warn("accept");
                return;
            }
            
            pfds.push_back({ conn, POLLIN | POLLPRI });
            rbufs.insert(std::make_pair(conn, recvbuf()));
        }
    }
}

