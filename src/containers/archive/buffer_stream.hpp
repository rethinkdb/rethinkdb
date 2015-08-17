// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_BUFFER_STREAM_HPP_
#define CONTAINERS_ARCHIVE_BUFFER_STREAM_HPP_

#include <string.h>

#include "containers/archive/archive.hpp"

// Reads from a buffer without taking ownership over it
class buffer_read_stream_t : public read_stream_t {
public:
    // Note: These are implemented in the header file because inlining them in
    // combination with `deserialize_varint_uint64()` yields significant performance
    // gains due to its frequent use in datum_string_t and other datum
    // deserialization functions. (measured on GCC 4.8)
    // If we end up compiling with whole-program link-time optimization some day,
    // we can probably move this back to a .cc file.

    explicit buffer_read_stream_t(const char *buf, size_t size, int64_t offset = 0)
        : pos_(offset), buf_(buf), size_(size) {
        guarantee(pos_ >= 0);
        guarantee(static_cast<uint64_t>(pos_) <= size_);
    }
    virtual ~buffer_read_stream_t() { }

    virtual MUST_USE int64_t read(void *p, int64_t n) {
        int64_t num_left = size_ - pos_;
        int64_t num_to_read = n < num_left ? n : num_left;

        memcpy(p, buf_ + pos_, num_to_read);

        pos_ += num_to_read;

        return num_to_read;
    }

    int64_t tell() const { return pos_; }

private:
    int64_t pos_;
    const char *buf_;
    size_t size_;

    DISABLE_COPYING(buffer_read_stream_t);
};


#endif  // CONTAINERS_ARCHIVE_BUFFER_STREAM_HPP_
