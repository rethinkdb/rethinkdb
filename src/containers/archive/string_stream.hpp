// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_STRING_STREAM_HPP_
#define CONTAINERS_ARCHIVE_STRING_STREAM_HPP_

#include <string>

#include "containers/archive/archive.hpp"

class read_string_stream_t : public read_stream_t {
public:
    explicit read_string_stream_t(const std::string &in);

    virtual MUST_USE int64_t read(void *p, int64_t n);

private:
    std::string str;
    int pos;

    DISABLE_COPYING(read_string_stream_t);
};

#endif  // CONTAINERS_ARCHIVE_STRING_STREAM_HPP_
