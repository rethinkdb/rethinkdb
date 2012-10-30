// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_FILE_STREAM_HPP_
#define CONTAINERS_ARCHIVE_FILE_STREAM_HPP_

#include <stdio.h>

#include "containers/archive/archive.hpp"

// This is a completely worthless type with BLOCKING I/O used only in
// the dummy protocol.
class blocking_read_file_stream_t : public read_stream_t {
public:
    blocking_read_file_stream_t();

    // Returns true upon success.
    MUST_USE bool init(const char *path);
    virtual ~blocking_read_file_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

private:
    FILE *fp_;

    DISABLE_COPYING(blocking_read_file_stream_t);
};


// This is a completely worthless type with BLOCKING I/O used only in
// the dummy protocol.
class blocking_write_file_stream_t : public write_stream_t {
public:
    blocking_write_file_stream_t();

    MUST_USE bool init(const char *path);
    virtual ~blocking_write_file_stream_t();

    virtual MUST_USE int64_t write(const void *p, int64_t n);

private:
    FILE *fp_;

    DISABLE_COPYING(blocking_write_file_stream_t);
};


#endif  // CONTAINERS_ARCHIVE_FILE_STREAM_HPP_
