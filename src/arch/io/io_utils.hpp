#ifndef ARCH_IO_IO_UTILS_HPP_
#define ARCH_IO_IO_UTILS_HPP_

#include "arch/runtime/runtime_utils.hpp"

/* Types of IO backends */
enum linux_io_backend_t {
    AIO_BACKEND_MIN_BOUND = 0, aio_native = 0, aio_default = 1, aio_pool = 1, AIO_BACKEND_MAX_BOUND = 1
};
typedef linux_io_backend_t io_backend_t;

// Thanks glibc for not providing a wrapper for this syscall :(
int _gettid();

/* scoped_fd_t is like scoped_ptr_t, but for a file descriptor */
class scoped_fd_t {
public:
    scoped_fd_t() : fd(INVALID_FD) { }
    explicit scoped_fd_t(fd_t f) : fd(f) { }
    ~scoped_fd_t() {
        reset(INVALID_FD);
    }
    fd_t reset(fd_t f2 = INVALID_FD);
    fd_t get() {
        return fd;
    }
    MUST_USE fd_t release() {
        fd_t f = fd;
        fd = INVALID_FD;
        return f;
    }
private:
    fd_t fd;
    DISABLE_COPYING(scoped_fd_t);
};

#endif /* ARCH_IO_IO_UTILS_HPP_ */
