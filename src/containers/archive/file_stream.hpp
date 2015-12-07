// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_FILE_STREAM_HPP_
#define CONTAINERS_ARCHIVE_FILE_STREAM_HPP_

// TODO ATN: is this class needed on windows?

#include <stdio.h>

#include "arch/io/io_utils.hpp"
#include "containers/archive/archive.hpp"

// This is a completely worthless type with BLOCKING I/O used only in
// http/file_app, json_import and for seeding randint()
class blocking_read_file_stream_t : public read_stream_t {
public:
    blocking_read_file_stream_t();

    // Returns true upon success.
    MUST_USE bool init(const char *path);

    // Returns true upon success, and more usefully, writes an errno
    // value to *errsv_out when returning false.
    MUST_USE bool init(const char *path, int *errsv_out);


    virtual ~blocking_read_file_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

private:
    scoped_fd_t fd_;

    DISABLE_COPYING(blocking_read_file_stream_t);
};


#endif  // CONTAINERS_ARCHIVE_FILE_STREAM_HPP_
