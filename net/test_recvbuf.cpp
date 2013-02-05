#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

#include "recvbuf.hpp"

int main(int argc, char **argv) {
    int fd;

    for (--argc, ++argv; argc > 0; --argc, ++argv) {
        fd = open(*argv, O_RDONLY);
        if (fd == -1) {
            warn("open: %s", *argv);
            continue;
        }

        recvbuf buf;

        buf.read(fd);
        while (buf.has_msg())
            std::cout << "str: " << buf.pop_msg() << std::endl;
        close(fd);
    }

    return 0;
}
