#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>

#include "recvbuf.hpp"
#include "json.hpp"

#define DEFAULT_PORT    1234
#define POLL_TIMEOUT    -1


void usage(void);
void server(unsigned short);


class App {
    public:
        virtual json::value process(const json::value &request) = 0;
        virtual ~App() = 0;
};

class Echoer : App {
    public:
        virtual json::value process(const json::value &request) {
            json::value response;

            response = request;
            response.erase("request");
            response["response"] = request["request"];

            return std::move(response);
        }
};

class User {
    public:
        User(const std::string &login, const std::string &password, const json::value &db) {
            // TODO
        }
};

struct Conn_ctx {
    User *user;
    App *app;
};

int
main(int argc, char **argv)
{
    int         ch;
    unsigned short     port = DEFAULT_PORT;
    extern char    *optarg;

    while ((ch = getopt(argc, argv, "hp:")) != -1) {
        switch (ch) {
        case 'h':
            usage();
            return 0;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            usage();
            return 1;
        }
    }

    server(port);

    return 0;
}

void
usage(void)
{
    fprintf(stderr, "usage: server [-p <port>]\n"
        "Default port is 1234.\n");
}

void
setnonblock(int fd)
{
    if (fcntl(fd, F_SETFD, O_NONBLOCK) == -1)
        err(1, "setnonblock: fcntl");
}

void
server(unsigned short port)
{
    int             s;
    int             conn;
    struct sockaddr_in     sa;
    struct             pfds;
    int             npoll;
    std::vector<pollfd>     pfds;
    std::map<int, recvbuf>     rbufs;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        err(1, "socket");
    setnonblock(s);
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *) &sa, sizeof sa) == -1)
        err(1, "bind");
    if (listen(s, 0) == -1)
        err(1, "listen");

    std::clog << "listening on port " << port << std::endl;

    pfds.push_back({ s, POLLIN });

    while ((npoll = poll(pfds.data(), pfds.size(), POLL_TIMEOUT)) != -1) {
        if (pfds[0].revents & POLLIN) {
            conn = accept(s, NULL, NULL);
            if (conn == -1) {
                warn("accept");
                continue;
            }
            std::clog << "accepted a new connection, fd=" << conn << std::endl;
            setnonblock(conn);
            pfds.push_back({ conn, POLLIN });
            continue;
        }

        std::vector<int> closed;
        for (pollfd *p = pfds.data() + 1; p < pfds.data() + pfds.size(); ++p) {
            std::clog << pfds.size() << " open fds" << std::endl;
            if (p->revents & POLLIN) {
                std::clog << "POLLIN occured for fd " << p->fd << std::endl;
                recvbuf &rbuf = rbufs[p->fd];
                if (rbuf.read(p->fd) == 0)
                    goto close;
                while (rbuf.has_msg()) {
                    json::value request(json::parse(rbuf.pop_msg()));
                    if (request["request"] == "hello") {
                        std::clog << "this is a hello request" << std::endl;
                    }
                    std::cout << p->fd << ": " << request.str() << std::endl;
                }
            } else if (p->revents & ~POLLIN) {
                std::clog << "not POLLIN occured for fd " << p->fd << std::endl;
close:
                if (close(p->fd) == -1)
                    warn("close");
                closed.push_back(p->fd);
            }
        }

        for (auto fd : closed) {
            std::clog << "erasing all traces of fd " << fd << std::endl;
            rbufs.erase(fd);
            pfds.erase(std::find_if(pfds.begin(), pfds.end(), [fd](const pollfd &p) { return p.fd == fd; }));
        }
    }
    if (npoll < 0)
        err(1, "server: poll");
}



