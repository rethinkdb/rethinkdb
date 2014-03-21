// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/buffer_group.hpp"

#include "containers/archive/buffer_group_stream.hpp"

buffer_group_read_stream_t::buffer_group_read_stream_t(const const_buffer_group_t *group)
    : group_(group), bufnum_(0), bufpos_(0) { }

buffer_group_read_stream_t::~buffer_group_read_stream_t() { }

int64_t buffer_group_read_stream_t::read(void *p, int64_t n) {
    char *v = static_cast<char *>(p);
    const int64_t original_n = n;

    while (bufnum_ < group_->num_buffers() && n > 0) {
        const_buffer_group_t::buffer_t buf = group_->get_buffer(bufnum_);
        int64_t bytes_to_copy = std::min(buf.size - bufpos_, n);
        memcpy(v, static_cast<const char *>(buf.data) + bufpos_, bytes_to_copy);
        n -= bytes_to_copy;
        v += bytes_to_copy;
        bufpos_ += bytes_to_copy;

        if (bufpos_ == buf.size) {
            ++bufnum_;
            bufpos_ = 0;
        }
    }

    return original_n - n;
}

bool buffer_group_read_stream_t::entire_stream_consumed() const {
    return bufnum_ == group_->num_buffers();
}

buffer_group_write_stream_t::buffer_group_write_stream_t(const buffer_group_t *group)
    : group_(group), bufnum_(0), bufpos_(0) { }

buffer_group_write_stream_t::~buffer_group_write_stream_t() { }

int64_t buffer_group_write_stream_t::write(const void *p, int64_t n) {
    const char *v = static_cast<const char *>(p);
    const int64_t original_n = n;

    while (bufnum_ < group_->num_buffers() && n > 0) {
        buffer_group_t::buffer_t buf = group_->get_buffer(bufnum_);
        int64_t bytes_to_copy = std::min(buf.size - bufpos_, n);
        memcpy(static_cast<char *>(buf.data) + bufpos_, v, bytes_to_copy);
        n -= bytes_to_copy;
        v += bytes_to_copy;
        bufpos_ += bytes_to_copy;

        if (bufpos_ == buf.size) {
            ++bufnum_;
            bufpos_ = 0;
        }
    }

    return n == 0 ? original_n : -1;
}

bool buffer_group_write_stream_t::entire_stream_filled() const {
    return bufnum_ == group_->num_buffers();
}
