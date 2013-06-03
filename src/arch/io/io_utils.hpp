// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_IO_UTILS_HPP_
#define ARCH_IO_IO_UTILS_HPP_

#include "arch/runtime/runtime_utils.hpp"

// Thanks glibc for not providing a wrapper for this syscall :(
int _gettid();

/* scoped_fd_t is like scoped_ptr_t, but for a file descriptor */
class scoped_fd_t {
public:
    scoped_fd_t() : fd(INVALID_FD) { }
    explicit scoped_fd_t(fd_t f) : fd(f) { }
    scoped_fd_t(scoped_fd_t &&other) : fd(other.fd) {
        other.fd = INVALID_FD;
    }

    ~scoped_fd_t() {
        reset(INVALID_FD);
    }

    scoped_fd_t &operator=(scoped_fd_t &&other) {
        scoped_fd_t tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    fd_t reset(fd_t f2 = INVALID_FD);

    // TODO: Make get check that it's not returning INVALID_FD.
    fd_t get() {
        return fd;
    }

    void swap(scoped_fd_t &other) {  // NOLINT(build/include_what_you_use)
        fd_t tmp = fd;
        fd = other.fd;
        other.fd = tmp;
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
