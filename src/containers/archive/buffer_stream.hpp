// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_BUFFER_STREAM_HPP_
#define CONTAINERS_ARCHIVE_BUFFER_STREAM_HPP_

#include "containers/archive/archive.hpp"

// Reads from a buffer without taking ownership over it
class buffer_read_stream_t : public read_stream_t {
public:
    explicit buffer_read_stream_t(const char *buf, size_t size, int64_t offset = 0);
    virtual ~buffer_read_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

    int64_t tell() const { return pos_; }

private:
    int64_t pos_;
    const char *buf_;
    size_t size_;

    DISABLE_COPYING(buffer_read_stream_t);
};


#endif  // CONTAINERS_ARCHIVE_BUFFER_STREAM_HPP_
