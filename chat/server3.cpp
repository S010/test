#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <err.h>
#include <poll.h>

#include <json.hpp>

static const int default_port = 1234;
static const int default_poll_timeout = 1; // in milliseconds

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

static void set_nonblock(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        err(1, "fcntl");
}

static int listen_port(int port) {
    int                     s;
    struct sockaddr_in      sa;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        err(1, "socket");
    //set_nonblock(s);

    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((short) port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1)
        err(1, "bind");
    if (listen(s, 0) == -1)
        err(1, "listen");

    return s;
}

static void repeat(int s, const std::string &msg, const std::vector<pollfd> &pfds) {
    for (auto i = pfds.begin() + 1; i != pfds.end(); ++i) {
        if (i->fd == s)
            continue;
        std::clog << "repeating msg to " << i->fd << std::endl;
        auto nbytes = write(i->fd, msg.c_str(), msg.size());
        std::clog << " wrote " << nbytes << std::endl;
        nbytes = write(i->fd, "\n", 1);
    }
}

static void server(int port) {
    int                     s;
    std::vector<pollfd>     pfds;

    s = listen_port(port);

    pfds.push_back({ s, POLLIN });

    for ( ;; ) {
        auto npoll = poll(pfds.data(), pfds.size(), -1);

        if (npoll == -1)
            err(1, "poll");
        else if (npoll == 0)
            continue;

        std::clog << "----" << std::endl
                  << "npoll=" << npoll << std::endl
                  << "pfds.size()=" << pfds.size() << std::endl;

        for (auto &i : pfds) {
            if (i.fd == s)
                continue;

            if (i.revents & POLLIN) {
                recvbuf &rbuf = rbufs[i.fd];
                if (rbuf.read(i.fd) == 0) { // Connection closed by remote host
                    i.revents |= POLLHUP;
                    close(i.fd);
                    rbufs.erase(i.fd);
                } else if (rbuf) {
                    const std::string msg(rbuf.str());
                    std::clog << "got message from " << i.fd << ": " << msg << std::endl;
                    repeat(i.fd, msg, pfds);
                }
            } else if (i.revents != 0) {
                std::clog << "closing fd " << i.fd << std::endl;
                close(i.fd);
                rbufs.erase(i.fd);
            }
        }
        pfds.erase(
            std::remove_if(
                pfds.begin(),
                pfds.end(),
                [](const pollfd &pfd) {
                    return pfd.revents != 0 && !(pfd.revents & POLLIN);
                }
            ),
            pfds.end()
        );

        // Accept the new connection if any
        auto &pfd = pfds[0];
        if (pfd.revents & (POLLIN | POLLPRI)) {
            auto conn = accept(pfd.fd, nullptr, nullptr);
            if (conn == -1) {
                warn("accept");
                continue;
            }
            //set_nonblock(conn);

            std::clog << "accepted new connection" << std::endl;

            pfds.push_back({ conn, POLLIN | POLLPRI });
            rbufs.insert(std::make_pair(conn, recvbuf()));
        }
    }
}


