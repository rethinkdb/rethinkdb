// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/archive/vector_stream.hpp"

#include <string.h>

vector_stream_t::vector_stream_t() { }

vector_stream_t::~vector_stream_t() { }

int64_t vector_stream_t::write(const void *p, int64_t n) {
    const char *chp = static_cast<const char *>(p);
    vec_.insert(vec_.end(), chp, chp + n);

    return n;
}

vector_read_stream_t::vector_read_stream_t(const std::vector<char> *vector) : pos_(0), vec_(vector) { }
vector_read_stream_t::~vector_read_stream_t() { }

int64_t vector_read_stream_t::read(void *p, int64_t n) {
    int64_t num_left = vec_->size() - pos_;
    int64_t num_to_read = n < num_left ? n : num_left;

    memcpy(p, vec_->data() + pos_, num_to_read);

    pos_ += num_to_read;

    return num_to_read;
}
