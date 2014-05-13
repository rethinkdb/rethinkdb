// Copyright 2010-2013 RethinkDB, all rights reserved.
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
    size_t size = 0;
    if (value == 0) {
        buf[0] = 0;
        size = 1;
    } else {
        uint64_t n = value;
        while (n >= (1 << 7)) {
            buf[size] = ((n & ((1 << 7) - 1)) | (1 << 7));
            ++size;
            n >>= 7;
        }
        buf[size] = n;
        ++size;
    }
    wm->append(buf, size);
}

archive_result_t deserialize_varint_uint64(read_stream_t *s, uint64_t *value_out) {
    uint64_t value = 0;

    int offset = 0;
    for (;;) {
        uint8_t buf[1];
        int64_t res = s->read(buf, 1);
        if (res == 1) {
            uint64_t x = (buf[0] & ((1 << 7) - 1));
            value |= (x << offset);
            if ((buf[0] & (1 << 7)) == 0) {
                if (offset == 63 && x > 1) {
                    return archive_result_t::RANGE_ERROR;
                } else {
                    *value_out = value;
                    return archive_result_t::SUCCESS;
                }
            }
            if (offset == 63) {
                return archive_result_t::RANGE_ERROR;
            }
            offset += 7;
        } else if (res == -1) {
            return archive_result_t::SOCK_ERROR;
        } else if (res == 0) {
            return archive_result_t::SOCK_EOF;
        } else {
            unreachable();
        }
    }
}

