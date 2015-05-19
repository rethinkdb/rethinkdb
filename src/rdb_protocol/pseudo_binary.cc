// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/pseudo_binary.hpp"

#include "errors.hpp"

#include "utils.hpp"
#include "rdb_protocol/base64.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"

#include "debug.hpp"

namespace ql {
namespace pseudo {

const char *const binary_string = "BINARY";
const char *const data_key = "data";

// Given a raw data string, encodes it into a `r.binary` pseudotype with base64 encoding
scoped_cJSON_t encode_base64_ptype(const datum_string_t &data) {
    scoped_cJSON_t res(cJSON_CreateObject());
    res.AddItemToObject(datum_t::reql_type_string.to_std().c_str(),
                        cJSON_CreateString(binary_string));
    res.AddItemToObject(
        data_key,
        cJSON_CreateString(encode_base64(data.data(), data.size()).c_str()));
    return res;
}

// Given a `r.binary` pseudotype with base64 encoding, decodes it into a raw data string
datum_string_t decode_base64_ptype(
        const std::vector<std::pair<datum_string_t, datum_t> > &ptype) {
    bool has_data = false;
    datum_string_t res;
    for (auto it = ptype.begin(); it != ptype.end(); ++it) {
        if (it->first == datum_t::reql_type_string) {
            r_sanity_check(it->second.as_str() == binary_string);
        } else if(it->first == data_key) {
            has_data = true;
            datum_string_t base64_data = it->second.as_str();
            std::string decoded_str = decode_base64(base64_data.data(),
                                                    base64_data.size());
            res = datum_string_t(decoded_str.size(), decoded_str.data());
        } else {
            rfail_datum(base_exc_t::GENERIC,
                        "Invalid binary pseudotype: illegal `%s` key.",
                        it->first.to_std().c_str());
        }
    }
    rcheck_datum(has_data, base_exc_t::GENERIC,
                 strprintf("Invalid binary pseudotype: lacking `%s` key.",
                           data_key).c_str());
    return res;
}

void write_binary_to_protobuf(Datum *d, const datum_string_t &data) {
    d->set_type(Datum::R_OBJECT);

    // Add pseudotype field with binary type
    Datum_AssocPair *ap = d->add_r_object();
    ap->set_key(datum_t::reql_type_string.to_std().c_str());
    ap->mutable_val()->set_type(Datum::R_STR);
    ap->mutable_val()->set_r_str(binary_string);

    // Add 'data' field with base64-encoded data
    std::string encoded_data(encode_base64(data.data(), data.size()));
    ap = d->add_r_object();
    ap->set_key(data_key);
    ap->mutable_val()->set_type(Datum::R_STR);
    ap->mutable_val()->set_r_str(encoded_data.data(), encoded_data.size());
}

} // namespace pseudo
} // namespace ql
