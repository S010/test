#ifndef RECVBUF_HPP
#define RECVBUF_HPP

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <stdexcept>

#include <unistd.h>

class recvbuf {
    public:
        recvbuf(char marker = '\n') :
            buf(nullptr),
            markerfound(false),
            bufsize(0),
            markerpos(0),
            readpos(0),
            marker(marker)
        {
        }

        ~recvbuf() {
            free(buf);
        }

        size_t read(int fd) {
            size_t sum = 0;
            for ( ;; ) {
                if (readpos >= bufsize)
                    grow();

                size_t readsize = bufsize - readpos;
                ssize_t nread = ::read(fd, buf + readpos, readsize);
                if (nread == -1) {
                    if (errno == EAGAIN)
                        break;
                    else
                        throw std::runtime_error("read(2) failed!");
                } else {
                    if (!markerfound)
                        markerfound = find(readpos, nread);
                    readpos += nread;
                    sum += (size_t) nread;
                    if ((size_t) nread < readsize)
                        break;
                }
            }

            return sum;
        }

        bool has_msg() {
            return markerfound;
        }

        std::string pop_msg() {
            if (!markerfound)
                return "";
            std::string s(buf, buf + markerpos);
            discard(markerpos);
            if (readpos > 0)
                markerfound = find(0, readpos - 1);
            else
                markerfound = false;
            return s;
        }

    private:
        char                *buf;
        bool                 markerfound;
        size_t               bufsize;
        size_t               markerpos;
        size_t               readpos;
        static const size_t  growsize = 8192;
        const char           marker;

        void grow() {
            resize(bufsize + growsize);
        }

        void resize(size_t size) {
            char *p = (char *) realloc(buf, size);
            if (p == nullptr)
                throw std::bad_alloc();
            bufsize = size;
            buf = p;
        }

        void discard(size_t pos) {
            memmove(buf, buf + pos + 1, readpos - pos - 1);
            readpos -= (pos + 1);
            resize((readpos / growsize + 1) * growsize);
        }

        bool find(size_t start, size_t size) {
            char *p = (char *) memchr(buf + start, marker, size);
            if (p == nullptr) {
                return false;
            } else {
                markerpos = p - buf;
                return true;
            }
        }
};

#endif
