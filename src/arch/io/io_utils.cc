// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/io_utils.hpp"

#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

#include "logger.hpp"

int _gettid() {
    return syscall(SYS_gettid);
}

fd_t scoped_fd_t::reset(fd_t f2) {
    if (fd != INVALID_FD) {
        int res = close(fd);
        if (res != 0) {
            logERR("Error in close(): %s", strerror(errno));
        }
    }
    fd = f2;
    return f2;
}
