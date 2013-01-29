#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "recvbuf.hpp"

int main(int argc, char **argv) {
    for (--argc, ++argv; argc > 0; --argc, ++argv) {
        int fd = open(*argv, O_RDONLY);
        if (fd == -1) {
            perror(*argv);
            continue;
        }
        recvbuf rbuf;
        rbuf.read(fd);
        close(fd);
        while (rbuf.hasmsg()) {
            std::cout << "msg: \"" << rbuf.popmsg() << "\"" << std::endl;
        }
    }
    return 0;
}
