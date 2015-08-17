// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_STRING_STREAM_HPP_
#define CONTAINERS_ARCHIVE_STRING_STREAM_HPP_

#include <string>

#include "containers/archive/archive.hpp"

class string_stream_t : public write_stream_t {
public:
    string_stream_t();
    virtual ~string_stream_t();

    virtual MUST_USE int64_t write(const void *p, int64_t n);

    std::string &str() { return str_; }

private:
    std::string str_;

    DISABLE_COPYING(string_stream_t);
};

class string_read_stream_t : public read_stream_t {
public:
    explicit string_read_stream_t(std::string &&_source, int64_t _offset);
    virtual ~string_read_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

    void swap(std::string *other_source, int64_t *other_offset);

private:
    std::string source;
    int64_t offset;

    DISABLE_COPYING(string_read_stream_t);
};

#endif  // CONTAINERS_ARCHIVE_STRING_STREAM_HPP_
