// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/archive/vector_stream.hpp"

#include <string.h>

vector_stream_t::vector_stream_t() { }

vector_stream_t::~vector_stream_t() { }

int64_t vector_stream_t::write(const void *p, int64_t n) {
    const char *chp = static_cast<const char *>(p);
    vec_.insert(vec_.end(), chp, chp + n);

    return n;
}

void vector_stream_t::swap(std::vector<char> *other) {
    other->swap(vec_);
}

void vector_stream_t::reserve(size_t reserve_size) {
    vec_.reserve(reserve_size);
}

vector_read_stream_t::vector_read_stream_t(std::vector<char> &&vector, int64_t offset)
    : pos_(offset), vec_(std::move(vector)) {
    guarantee(pos_ >= 0);
    guarantee(static_cast<uint64_t>(pos_) <= vec_.size());
}
vector_read_stream_t::~vector_read_stream_t() { }

int64_t vector_read_stream_t::read(void *p, int64_t n) {
    int64_t num_left = vec_.size() - pos_;
    int64_t num_to_read = n < num_left ? n : num_left;

    memcpy(p, vec_.data() + pos_, num_to_read);

    pos_ += num_to_read;

    return num_to_read;
}

void vector_read_stream_t::swap(std::vector<char> *other_vec, int64_t *other_pos) {
    int64_t temp_pos = *other_pos;
    *other_pos = pos_;
    pos_ = temp_pos;

    vec_.swap(*other_vec);

    guarantee(pos_ >= 0);
    guarantee(static_cast<uint64_t>(pos_) <= vec_.size());
}
