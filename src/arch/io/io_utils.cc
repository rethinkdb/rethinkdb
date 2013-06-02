// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/io_utils.hpp"

#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

#include "logger.hpp"

int _gettid() {
#ifdef __FreeBSD__
    return getpid();
#else
    return syscall(SYS_gettid);
#endif
}

fd_t scoped_fd_t::reset(fd_t f2) {
    if (fd != INVALID_FD) {
        int res = close(fd);
        guarantee_err(res == 0, "error returned by close(2)");
    }
    fd = f2;
    return f2;
}
