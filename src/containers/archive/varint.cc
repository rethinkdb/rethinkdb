// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/archive/varint.hpp"

size_t varint_uint64_serialized_size(uint64_t value) {
    size_t count = 0;

    ++count;
    value /= 128;

    while (value > 0) {
        value /= 128;
        ++count;
    }

    return count;
}

void serialize_varint_uint64(write_message_t *wm, const uint64_t value) {
    // buf needs to be 10 or more -- ceil(64/7) is 10.
    uint8_t buf[16];
    size_t size = serialize_varint_uint64_into_buf(value, buf);
    wm->append(buf, size);
}

size_t serialize_varint_uint64_into_buf(const uint64_t value, uint8_t *buf_out) {
    size_t size = 0;
    if (value == 0) {
        buf_out[0] = 0;
        size = 1;
    } else {
        uint64_t n = value;
        while (n >= (1 << 7)) {
            buf_out[size] = ((n & ((1 << 7) - 1)) | (1 << 7));
            ++size;
            n >>= 7;
        }
        buf_out[size] = n;
        ++size;
    }
    return size;
}
