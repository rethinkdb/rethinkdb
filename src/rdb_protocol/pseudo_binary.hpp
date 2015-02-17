// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_BINARY_HPP_
#define RDB_PROTOCOL_PSEUDO_BINARY_HPP_

#include <utility>
#include <vector>

#include "containers/scoped.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rdb_protocol/datum_string.hpp"
#include "rdb_protocol/datum.hpp"

namespace ql {
namespace pseudo {

extern const char *const binary_string;
extern const char *const data_key;

// Given a raw data string, encodes it into a `r.binary` pseudotype with base64 encoding
void encode_base64_ptype(
        const datum_string_t &data,
        rapidjson::Writer<rapidjson::StringBuffer> *writer);
void write_binary_to_protobuf(Datum *d, const datum_string_t &data);

// Given a `r.binary` pseudotype with base64 encoding, decodes it into a raw data string
datum_string_t decode_base64_ptype(
    const std::vector<std::pair<datum_string_t, datum_t> > &ptype);

} // namespace pseudo
} // namespace ql

#endif  // RDB_PROTOCOL_PSEUDO_BINARY_HPP_
