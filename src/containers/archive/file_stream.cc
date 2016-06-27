// Copyright 2010-2012 RethinkDB, all rights reserved.

#include "containers/archive/file_stream.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

blocking_read_file_stream_t::blocking_read_file_stream_t() { }

blocking_read_file_stream_t::~blocking_read_file_stream_t() { }

bool blocking_read_file_stream_t::init(const char *path, int *errsv_out) {
    guarantee(fd_.get() == INVALID_FD);

#ifdef _WIN32
    HANDLE h = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        logWRN("CreateFile failed: %s", winerr_string(GetLastError()).c_str());
        *errsv_out = EIO;
        return false;
    } else {
        *errsv_out = 0;
        fd_.reset(h);
        return true;
    }
#else
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
#endif
}


bool blocking_read_file_stream_t::init(const char *path) {
    // This version of init simply doesn't provide the errno value to
    // the caller.
    int errsv;
    return init(path, &errsv);
}

int64_t blocking_read_file_stream_t::read(void *p, int64_t n) {
    guarantee(fd_.get() != INVALID_FD);

#ifdef _WIN32
    DWORD read_bytes;
    BOOL res = ReadFile(fd_.get(), p, n, &read_bytes, NULL);
    if (!res) {
        logERR("ReadFile failed: %s", winerr_string(GetLastError()).c_str());
        set_errno(EIO);
        return -1;
    } else {
        return read_bytes;
    }
#else
    ssize_t res;
    do {
        res = ::read(fd_.get(), p, n);
    } while (res == -1 && get_errno() == EINTR);

    return res;
#endif
}

