
#ifndef __IO_CALLS_HPP__
#define __IO_CALLS_HPP__

#include <unistd.h>
#include <stdio.h>
#include "arch/resource.hpp"

// TODO: AIO API should probably be moved here as well.

struct posix_io_calls_t {
    ssize_t read(resource_t fd, void *buf, size_t count) {
        return ::read(fd, buf, count);
    }

    ssize_t write(resource_t fd, const void *buf, size_t count) {
        return ::write(fd, buf, count);
    }
};

#endif // __IO_CALLS_HPP__

