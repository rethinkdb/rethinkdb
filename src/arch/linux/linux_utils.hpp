
#ifndef __ARCH_LINUX_UTILS_HPP__
#define __ARCH_LINUX_UTILS_HPP__

#include "errors.hpp"

// Thanks glibc for not providing a wrapper for this syscall :(
int _gettid();

typedef int fd_t;
#define INVALID_FD fd_t(-1)

/* scoped_fd_t is like boost::scoped_ptr, but for a file descriptor */
class scoped_fd_t {
public:
    scoped_fd_t() : fd(INVALID_FD) { }
    scoped_fd_t(fd_t f) : fd(f) { }
    ~scoped_fd_t() {
        reset(INVALID_FD);
    }
    fd_t reset(fd_t f2 = INVALID_FD);
    fd_t get() {
        return fd;
    }
    fd_t release() {
        fd_t f = fd;
        fd = INVALID_FD;
        return f;
    }
private:
    fd_t fd;
    DISABLE_COPYING(scoped_fd_t);
};

#endif // __ARCH_LINUX_UTILS_HPP__

