// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifdef _WIN32

// TODO WINDOWS

#else

#include "containers/archive/file_stream.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

blocking_read_file_stream_t::blocking_read_file_stream_t() { }

blocking_read_file_stream_t::~blocking_read_file_stream_t() { }

bool blocking_read_file_stream_t::init(const char *path, int *errsv_out) {
    guarantee(fd_.get() == INVALID_FD);

    int res;
    do {
        res = open(path, O_RDONLY);
    } while (res == -1 && get_errno() == EINTR);

    if (res == -1) {
        *errsv_out = get_errno();
        return false;
    } else {
        fd_.reset(res);
        *errsv_out = 0;
        return true;
    }
}


bool blocking_read_file_stream_t::init(const char *path) {
    // This version of init simply doesn't provide the errno value to
    // the caller.
    int errsv;
    return init(path, &errsv);
}

int64_t blocking_read_file_stream_t::read(void *p, int64_t n) {
    guarantee(fd_.get() != INVALID_FD);

    ssize_t res;
    do {
        res = ::read(fd_.get(), p, n);
    } while (res == -1 && get_errno() == EINTR);

    return res;
}

#endif
