// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/io/io_utils.hpp"

#ifndef _WIN32 // TODO ATN
#include <sys/syscall.h>
#else
#include "windows.hpp"
#endif
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include <algorithm>

#include "logger.hpp"

#ifndef _WIN32 // TODO ATN
int _gettid() {
    return syscall(SYS_gettid);
}
#endif

fd_t scoped_fd_t::reset(fd_t f2) {
    if (fd != INVALID_FD) {
#ifdef _WIN32 // ATN TODO
        BOOL res = CloseHandle(fd);
        guarantee_winerr(res != 0, "CloseHandle failed");
#else
        int res;
        do {
            res = close(fd);
        } while (res != 0 && get_errno() == EINTR);
        guarantee_err(res == 0, "error returned by close(2)");
#endif
    }
    fd = f2;
    return f2;
}

// This completely fills dest_vecs with data from source_vecs (starting at
// offset_into_source).  Crashes if source_vecs is too small!
void fill_bufs_from_source(iovec *dest_vecs, const size_t dest_size,
                           iovec *source_vecs, const size_t source_size,
                           const size_t offset_into_source) {

    // dest_byte always less than dest_vecs[dest_buf].iov_len. Same is true of
    // source.
    size_t dest_buf = 0;
    size_t dest_byte = 0;
    size_t source_buf = 0;
    size_t source_byte = 0;

    // Walk source forward to the offset.
    for (size_t n = offset_into_source; ; ) {
        guarantee(source_buf < source_size);
        if (n < source_vecs[source_buf].iov_len) {
            source_byte = n;
            break;
        } else {
            n -= source_vecs[source_buf].iov_len;
            ++source_buf;
        }
    }

    // Copy bytes until dest is filled.
    while (dest_buf < dest_size) {
        guarantee(source_buf < source_size);
        size_t copy_size = std::min(dest_vecs[dest_buf].iov_len - dest_byte,
                                    source_vecs[source_buf].iov_len - source_byte);

        memcpy(static_cast<char *>(dest_vecs[dest_buf].iov_base) + dest_byte,
               static_cast<char *>(source_vecs[source_buf].iov_base) + source_byte,
               copy_size);

        dest_byte += copy_size;
        if (dest_byte == dest_vecs[dest_buf].iov_len) {
            ++dest_buf;
            dest_byte = 0;
        }

        source_byte += copy_size;
        if (source_byte == source_vecs[source_buf].iov_len) {
            ++source_buf;
            source_byte = 0;
        }
    }
}
