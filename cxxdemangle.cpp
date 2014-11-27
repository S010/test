#include <cxxabi.h>
#include <iostream>

int main(int argc, char **argv) {
    char* p;
    char buf[8192];
    size_t size;
    int status;

    while (++argv, --argc) {
        *buf = '\0';
        size = sizeof buf - 1;
        p = __cxxabiv1::__cxa_demangle(*argv, buf, &size, &status);
        buf[size] = '\0';
        std::cout << buf << std::endl;
    }
    return 0;
}
