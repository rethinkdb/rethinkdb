// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_BINARY_HPP_
#define RDB_PROTOCOL_PSEUDO_BINARY_HPP_

#include <map>
#include <string>

#include "containers/wire_string.hpp"
#include "rdb_protocol/datum.hpp"

namespace ql {
namespace pseudo {

extern const char *const binary_string;
extern const char *const data_key;

// Given a raw data string, encodes it into a `r.binary` pseudotype with base64 encoding
scoped_cJSON_t encode_base64_ptype(const wire_string_t &data);
void write_binary_to_protobuf(Datum *d, const wire_string_t &data);

// Given a `r.binary` pseudotype with base64 encoding, decodes it into a raw data string
scoped_ptr_t<wire_string_t> decode_base64_ptype(
    const std::map<std::string, counted_t<const datum_t> > &ptype);

} // namespace pseudo
} // namespace ql

#endif  // RDB_PROTOCOL_PSEUDO_BINARY_HPP_
