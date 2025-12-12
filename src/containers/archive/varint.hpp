// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file varint.hpp
/// @brief Variable-length integer encoding/decoding
///
/// Implements variable-length integer (varint) encoding for efficient storage
/// of integers. Values are encoded in base-128 with the MSB of each byte
/// indicating if more bytes follow.
///
/// This encoding is more efficient than fixed-width integers for small values,
/// making it ideal for network protocols and storage.
///
/// @defgroup Serialization
/// @{

#ifndef CONTAINERS_ARCHIVE_VARINT_HPP_
#define CONTAINERS_ARCHIVE_VARINT_HPP_

#include "containers/archive/archive.hpp"

/// @brief Calculates the serialized size of a varint uint64
///
/// Determines how many bytes are needed to encode the given value using
/// varint encoding.
///
/// @param value The value to calculate size for
/// @return Number of bytes required to encode value
/// @example
/// @code
/// uint64_t num = 138;
/// size_t size = varint_uint64_serialized_size(num);  // Returns 2
/// @endcode
///
/// Example encodings:
/// - 1     -> 1 byte:  [0000 0001]
/// - 127   -> 1 byte:  [0111 1111]
/// - 128   -> 2 bytes: [1000 0000, 0000 0001]
/// - 138   -> 2 bytes: [1000 1010, 0000 0001]
size_t varint_uint64_serialized_size(uint64_t value);

/// @brief Serializes a uint64 as a varint to a write message
/// Uses variable-length encoding to minimize bytes for small values.
/// @param wm The write message to serialize into
/// @param value The value to serialize
void serialize_varint_uint64(write_message_t *wm, const uint64_t value);

/// @brief Serializes a uint64 varint directly into a buffer
/// Encodes the value directly into provided buffer without using write_message_t.
/// @param value The value to serialize
/// @param buf_out Buffer to write the varint into
/// @return Number of bytes actually written
/// @pre buf_out must have size >= varint_uint64_serialized_size(value)
size_t serialize_varint_uint64_into_buf(const uint64_t value, uint8_t *buf_out);

/// @brief Deserializes a uint64 from varint encoding
/// Reads and decodes a varint from a stream, supporting values up to 2^64-1.
/// Unlike Protocol Buffers, this doesn't silently truncate out-of-range values.
/// @param s The read stream to deserialize from
/// @param value_out Pointer to receive the decoded value
/// @return Result status (SUCCESS, SOCK_ERROR, SOCK_EOF, or RANGE_ERROR)
/// @example
/// @code
/// uint64_t decoded;
/// archive_result_t res = deserialize_varint_uint64(stream, &decoded);
/// throw_if_bad_deserialization(res, "varint");
/// @endcode
///
/// This function is inlined for performance in datum deserialization paths.
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
