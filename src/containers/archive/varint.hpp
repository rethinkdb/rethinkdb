// Copyright 2010-2013 RethinkDB, all rights reserved.
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
archive_result_t deserialize_varint_uint64(read_stream_t *s, uint64_t *value_out);

#endif  // CONTAINERS_ARCHIVE_VARINT_HPP_
