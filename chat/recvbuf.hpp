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
        recvbuf(size_t growsize = 8192, const std::string &sep) :
            buf(nullptr),
            nlfound(false),
            bufsize(0),
            nl(0),
            readi(0),
            growsize(growsize),
            msg_sep(msg_sep)
        {
            std::clog << __func__ << std::endl;
        }

        ~recvbuf() {
            std::clog << __func__ << std::endl;
        }

        size_t read(int fd) {
            std::clog << __func__ << std::endl;
            size_t sum = 0;
            for ( ;; ) {
                if (readi >= bufsize)
                    grow_buf();

                size_t readsize = bufsize - readi;
                std::clog << " trying to read " << readsize << " bytes" << std::endl;
                ssize_t nbytes = ::read(fd, buf + readi, readsize);
                if (nbytes == -1) {
                    if (errno == EAGAIN)
                        break;
                    else
                        throw std::runtime_error("read(2) failed!");
                } else {
                    if (!nlfound)
                        nlfound = find_sep(readi, nbytes);
                    readi += nbytes;
                    sum += (size_t) nbytes;
                    if ((size_t) nbytes < readsize)
                        break;
                }
            }

            return sum;
        }

        operator bool () {
            std::clog << __func__ << std::endl;
            return nlfound;
        }

        std::string str() {
            std::clog << __func__ << std::endl;
            if (!nlfound)
                return "";
            std::string s(buf, buf + nl);
            discard_before(nl);
            if (readi > 0)
                nlfound = find_sep(0, readi - 1);
            else
                nlfound = false;
            return s;
        }

    private:
        char              *buf;
        bool               nlfound;
        size_t             bufsize;
        size_t             nl;
        size_t             readi;
        const size_t       growsize;
        const std::string  sep;

        void grow_buf() {
            std::clog << __func__ << std::endl;
            resize_buf(bufsize + growsize);
        }

        void resize_buf(size_t size) {
            std::clog << __func__ << std::endl;
            char *p = (char *) realloc(buf, size);
            if (p == nullptr)
                throw std::bad_alloc();
            bufsize = size;
            buf = p;
        }

        void discard_before(size_t pos) {
            std::clog << __func__ << std::endl;
            memmove(buf, buf + pos + 1, readi - pos - 1);
            readi -= (pos + 1);
            resize_buf((readi / growsize + 1) * growsize);
        }

        bool find_sep(size_t start, size_t size) {
            char *p = (char *) memchr(buf + start, '\n', size);
            if (p == nullptr) {
                return false;
            } else {
                nl = p - buf;
                return true;
            }
        }
};


#endif
