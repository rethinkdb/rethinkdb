// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_VARINT_HPP_
#define CONTAINERS_ARCHIVE_VARINT_HPP_

#include "containers/archive/archive.hpp"

// We don't use google::protobuf::io::CodedOutputStream::WriteVarint64ToArray because
// we have no clean way of _reading_ varints without constructing a CodedInputStream,
// which would require knowing the varint structure anyway in order to know how many
// bytes of the input stream to read off and load into an array.

// Varint encoding encodes a number in base-128 in little-endian order, with the MSB
// the last byte 0, all other bytes 1.
//
// Some sample encodings:
//
// 1     -> [0000 0001]
// 127   -> [0111 1111]
// 138   -> [1000 1010, 0000 0001]
//
// Unlike protocol buffers does (or what its documentation claims it does), we don't
// silently truncate out-of-range varints when decoding.

size_t varint_uint64_serialized_size(uint64_t value);

void serialize_varint_uint64(write_message_t *wm, const uint64_t value);
// buf_out must have a size of at least varint_uint64_serialized_size(value).
// The actually used size is returned.
size_t serialize_varint_uint64_into_buf(const uint64_t value, uint8_t *buf_out);

// Note: This is defined in the header file because inlining this yields
// significant performance gains due to its frequent use in datum_string_t and
// other datum deserialization functions. (measured on GCC 4.8)
// If we end up compiling with whole-program link-time optimization some day,
// we can probably move this back to the .cc file.
inline archive_result_t deserialize_varint_uint64(read_stream_t *s, uint64_t *value_out) {
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
            unreachable("deserialize_varint_uint64: read: unexpected result");
        }
    }
}

#endif  // CONTAINERS_ARCHIVE_VARINT_HPP_
