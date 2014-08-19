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

counted_t<const ql::datum_t> convert_db_and_table_to_datum(
        const name_string_t &database, const name_string_t &table);
bool convert_db_and_table_from_datum(
        counted_t<const ql::datum_t> datum,
        name_string_t *database_out,
        name_string_t *table_out,
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

template<class T>
counted_t<const ql::datum_t> convert_set_to_datum(
        const std::function<counted_t<const ql::datum_t>(const T&)> &conv,
        const std::set<T> &set) {
    ql::datum_array_builder_t builder((ql::configured_limits_t()));
    builder.reserve(set.size());
    for (const T &elem : set) {
        builder.add(conv(elem));
    }
    return std::move(builder).to_counted();
}

template<class T>
bool convert_set_from_datum(
        const std::function<bool(counted_t<const ql::datum_t>, T*, std::string*)> &conv,
        bool allow_duplicates,
        counted_t<const ql::datum_t> datum,
        std::set<T> *set_out,
        std::string *error_out) {
    if (datum->get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "Expected an array, got " + datum->print();
        return false;
    }
    set_out->clear();
    for (size_t i = 0; i < datum->size(); ++i) {
        T value;
        if (!conv(datum->get(i), &value, error_out)) {
            return false;
        }
        auto res = set_out->insert(value);
        if (!allow_duplicates && !res.second) {
            *error_out = datum->get(i)->print() + " was specified more than once.";
            return false;
        }
    }
    return true;
}

/* `converter_from_datum_object_t` is a helper for converting a `datum_t` to some other
type when the type's datum representation is an object with a fixed set of fields.
Construct a `converter_from_datum_object_t` and call `init()` with your datum. `init()`
will fail if the input isn't an object. Next, call `get()` or `get_optional()` for each
field of the object; `get()` will fail if the field isn't present. Finally, call
`check_no_extra_keys()`, which will fail if the object has any keys that you didn't call
`get()` or `get_optional()` for. This way, it will produce a nice error message if the
user passes an object with an invalid key. */
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

