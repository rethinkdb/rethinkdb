// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/archive/file_stream.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

blocking_read_file_stream_t::blocking_read_file_stream_t() { }

blocking_read_file_stream_t::~blocking_read_file_stream_t() { }

bool blocking_read_file_stream_t::init(const char *path) {
    guarantee(fd_.get() == INVALID_FD);

    int res;
    do {
        res = open(path, O_RDONLY);
    } while (res == -1 && errno == EINTR);

    if (res == -1) {
        return false;
    } else {
        fd_.reset(res);
        return true;
    }
}

int64_t blocking_read_file_stream_t::read(void *p, int64_t n) {
    guarantee(fd_.get() != INVALID_FD);

    ssize_t res;
    do {
        res = ::read(fd_.get(), p, n);
    } while (res == -1 && errno == EINTR);

    return res;
}
