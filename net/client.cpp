#include <cstring>
#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>

#include "recvbuf.hpp"

static void client(const std::string &ipstr, int port, const std::string &username);

int main(int argc, char **argv) {
    std::string ipstr("127.0.0.1");
    int port = 1234;
    std::string username(getenv("USER"));

    int ch;
    extern char *optarg;
    while ((ch = getopt(argc, argv, "i:p:u:")) != -1) {
        switch (ch) {
        case 'i':
            ipstr = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'u':
            username = optarg;
            break;
        default:
            std::cerr << "usage: client [-i <ip_addr>] [-p <port>] [-u <username>]" << std::endl;
        }
    }

    client(ipstr, port, username);
    
    return 0;
}

static int connect_to(const std::string &ipstr, int port) {
    in_addr in;
    int s;
    sockaddr_in sa;

    std::clog << "connecting to " << ipstr << ":" << port << std::endl;

    if (inet_aton(ipstr.c_str(), &in) == -1)
        errx(1, "invalid ip address specification");

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        err(1, "socket");

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr = in;
    if (connect(s, (sockaddr *) &sa, sizeof(sa)) == -1)
        err(1, "connect");

    return s;
}

static void set_nonblock(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        err(1, "fcntl");
}

static void client(const std::string &ipstr, int port, const std::string &username) {
    int s = connect_to(ipstr, port);
    pollfd pfds[2];
    recvbuf rbufs[2];

    pfds[0] = { STDIN_FILENO, POLLIN };
    pfds[1] = { s, POLLIN };

    set_nonblock(STDIN_FILENO);

    for ( ;; ) {
        auto npoll = poll(pfds, 1, -1);

        if (npoll == -1)
            err(1, "poll");
        else if (npoll == 0)
            continue;

        if (pfds[0].revents & POLLIN) {
            rbufs[0].read(STDIN_FILENO);
            while (rbufs[0].has_msg()) {
                const std::string msg(rbufs[0].pop_msg());
                auto nwrote = write(pfds[1].fd, msg.c_str(), msg.size());
                if (nwrote == -1)
                    err(1, "write");
                nwrote = write(pfds[1].fd, "\n", 1);
                if (nwrote == -1)
                    err(1, "write");
            }
        }
        if (pfds[1].revents & POLLIN) {
            rbufs[1].read(pfds[1].fd);
            while (rbufs[1].has_msg())
                std::cout << rbufs[1].pop_msg() << std::endl;
        }
    }
}
