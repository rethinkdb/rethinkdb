// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/archive/buffer_stream.hpp"

#include <string.h>

buffer_read_stream_t::buffer_read_stream_t(
        const char *buf, size_t size, int64_t offset)
    : pos_(offset), buf_(buf), size_(size) {
    guarantee(pos_ >= 0);
    guarantee(static_cast<uint64_t>(pos_) <= size_);
}
buffer_read_stream_t::~buffer_read_stream_t() { }

int64_t buffer_read_stream_t::read(void *p, int64_t n) {
    int64_t num_left = size_ - pos_;
    int64_t num_to_read = n < num_left ? n : num_left;

    memcpy(p, buf_ + pos_, num_to_read);

    pos_ += num_to_read;

    return num_to_read;
}
