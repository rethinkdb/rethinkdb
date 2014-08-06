// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_DATUM_ADAPTER_HPP_
#define CLUSTERING_ADMINISTRATION_DATUM_ADAPTER_HPP_

#include <set>
#include <string>
#include <vector>

#include "containers/name_string.hpp"
#include "rdb_protocol/datum.hpp"

counted_t<const ql::datum_t> convert_name_to_datum(
        const name_string_t &value);
bool convert_name_from_datum(
        counted_t<const ql::datum_t> datum,
        const std::string &what,   /* e.g. "server name" or "table name" */
        name_string_t *value_out,
        std::string *error_out);

counted_t<const ql::datum_t> convert_uuid_to_datum(
        const uuid_u &value);
bool convert_uuid_from_datum(
        counted_t<const ql::datum_t> datum,
        uuid_u *value_out,
        std::string *error_out);

template<class T>
counted_t<const ql::datum_t> convert_vector_to_datum(
        const std::function<counted_t<const ql::datum_t>(const T&)> &conv,
        const std::vector<T> &vector) {
    ql::datum_array_builder_t builder((ql::configured_limits_t()));
    builder.reserve(vector.size());
    for (const T &elem : vector) {
        builder.add(conv(elem));
    }
    return std::move(builder).to_counted();
}

template<class T>
bool convert_vector_from_datum(
        const std::function<bool(counted_t<const ql::datum_t>, T*, std::string*)> &conv,
        counted_t<const ql::datum_t> datum,
        std::vector<T> *vector_out,
        std::string *error_out) {
    if (datum->get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "Expected an array, got " + datum->print();
        return false;
    }
    vector_out->resize(datum->size());
    for (size_t i = 0; i < datum->size(); ++i) {
        if (!conv(datum->get(i), &(*vector_out)[i], error_out)) {
            return false;
        }
    }
    return true;
}

class converter_from_datum_object_t {
public:
    bool init(counted_t<const ql::datum_t> datum,
              std::string *error_out);
    bool get(const std::string &key,
             counted_t<const ql::datum_t> *value_out,
             std::string *error_out);
    void get_optional(const std::string &key,
                      counted_t<const ql::datum_t> *value_out);
    bool check_no_extra_keys(std::string *error_out);
private:
    counted_t<const ql::datum_t> datum;
    std::set<std::string> extra_keys;
};

#endif /* CLUSTERING_ADMINISTRATION_DATUM_ADAPTER_HPP_ */

