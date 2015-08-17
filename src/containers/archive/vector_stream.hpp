// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_VECTOR_STREAM_HPP_
#define CONTAINERS_ARCHIVE_VECTOR_STREAM_HPP_

#include <vector>

#include "containers/archive/archive.hpp"

class vector_stream_t : public write_stream_t {
public:
    vector_stream_t();
    virtual ~vector_stream_t();

    virtual MUST_USE int64_t write(const void *p, int64_t n);

    const std::vector<char> &vector() { return vec_; }

    void swap(std::vector<char> *other);

    void reserve(size_t reserve_size);

private:
    std::vector<char> vec_;

    DISABLE_COPYING(vector_stream_t);
};

class vector_read_stream_t : public read_stream_t {
public:
    explicit vector_read_stream_t(std::vector<char> &&vector, int64_t offset = 0);
    virtual ~vector_read_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);

    void swap(std::vector<char> *other_vec, int64_t *other_pos);

private:
    int64_t pos_;
    std::vector<char> vec_;

    DISABLE_COPYING(vector_read_stream_t);
};

#endif  // CONTAINERS_ARCHIVE_VECTOR_STREAM_HPP_
