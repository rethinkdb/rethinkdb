// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_DATUM_ADAPTER_HPP_
#define CLUSTERING_ADMINISTRATION_DATUM_ADAPTER_HPP_

#include <set>
#include <string>
#include <vector>

#include "containers/name_string.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/datum.hpp"
#include "time.hpp"

class cluster_semilattice_metadata_t;
class server_config_client_t;
class table_meta_client_t;

/* Note that we generally use `ql::configured_limits_t::unlimited` when converting
things to datum, rather than using a user-specified limit. This is mostly for consistency
with reading from a B-tree; if a value read from the B-tree contains an array larger than
the user-specified limit, then we don't throw an exception unless the user tries to grow
the array. Since these functions are used to construct values that the user will "read",
we use the same behavior here. This has the nice side effect that we don't have to worry
about threading `configured_limits_t` through these functions, or about handling
exceptions if the limit is violated. */

ql::datum_t convert_string_to_datum(
        const std::string &value);
bool convert_string_from_datum(
        const ql::datum_t &datum,
        std::string *value_out,
        std::string *error_out);

ql::datum_t convert_name_to_datum(
        const name_string_t &value);
bool convert_name_from_datum(
        ql::datum_t datum,
        const std::string &what,   /* e.g. "server name" or "table name" */
        name_string_t *value_out,
        std::string *error_out);

ql::datum_t convert_uuid_to_datum(
        const uuid_u &value);
bool convert_uuid_from_datum(
        ql::datum_t datum,
        uuid_u *value_out,
        std::string *error_out);

ql::datum_t convert_name_or_uuid_to_datum(
        const name_string_t &name,
        const uuid_u &uuid,
        admin_identifier_format_t identifier_format);

/* If the given server is connected, sets `*server_name_or_uuid_out` to a datum
representation of the server and returns `true`. If it's not connected, returns `false`.
*/
bool convert_connected_server_id_to_datum(
        const server_id_t &server_id,
        admin_identifier_format_t identifier_format,
        server_config_client_t *server_config_client,
        ql::datum_t *server_name_or_uuid_out,
        name_string_t *server_name_out);

/* `convert_table_id_to_datums()` will return `false` if the table ID corresponds to a
deleted table. If the table still exists but the database does not, it will return `true`
but set the database to `__deleted_database__`. */
bool convert_table_id_to_datums(
        const namespace_id_t &table_id,
        admin_identifier_format_t identifier_format,
        const cluster_semilattice_metadata_t &metadata,
        table_meta_client_t *table_meta_client,
        /* Any of these can be `nullptr` if they are not needed */
        ql::datum_t *table_name_or_uuid_out,
        name_string_t *table_name_out,
        ql::datum_t *db_name_or_uuid_out,
        name_string_t *db_name_out);

/* `convert_database_id_to_datum()` will return `false` if the database ID corresponds to
a deleted database. */
bool convert_database_id_to_datum(
        const database_id_t &db_id,
        admin_identifier_format_t identifier_format,
        const cluster_semilattice_metadata_t &metadata,
        ql::datum_t *db_name_or_uuid_out,
        name_string_t *db_name_out);
bool convert_database_id_from_datum(
        const ql::datum_t &db_name_or_uuid,
        admin_identifier_format_t identifier_format,
        const cluster_semilattice_metadata_t &metadata,
        database_id_t *db_id_out,
        name_string_t *db_name_out,
        std::string *error_out);

ql::datum_t convert_port_to_datum(
        uint16_t value);

ql::datum_t convert_microtime_to_datum(
        microtime_t value);

template<class T>
ql::datum_t convert_vector_to_datum(
        const std::function<ql::datum_t(const T&)> &conv,
        const std::vector<T> &vector) {
    ql::datum_array_builder_t builder((ql::configured_limits_t::unlimited));
    builder.reserve(vector.size());
    for (const T &elem : vector) {
        builder.add(conv(elem));
    }
    return std::move(builder).to_datum();
}

template<class T>
bool convert_vector_from_datum(
        const std::function<bool(ql::datum_t, T*, std::string*)> &conv,
        ql::datum_t datum,
        std::vector<T> *vector_out,
        std::string *error_out) {
    if (datum.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "Expected an array, got " + datum.print();
        return false;
    }
    vector_out->resize(datum.arr_size());
    for (size_t i = 0; i < datum.arr_size(); ++i) {
        if (!conv(datum.get(i), &(*vector_out)[i], error_out)) {
            return false;
        }
    }
    return true;
}

template<class T>
ql::datum_t convert_set_to_datum(
        const std::function<ql::datum_t(const T&)> &conv,
        const std::set<T> &set) {
    ql::datum_array_builder_t builder((ql::configured_limits_t::unlimited));
    builder.reserve(set.size());
    for (const T &elem : set) {
        builder.add(conv(elem));
    }
    return std::move(builder).to_datum();
}

template<class T>
bool convert_set_from_datum(
        const std::function<bool(ql::datum_t, T*, std::string*)> &conv,
        bool allow_duplicates,
        ql::datum_t datum,
        std::set<T> *set_out,
        std::string *error_out) {
    if (datum.get_type() != ql::datum_t::R_ARRAY) {
        *error_out = "Expected an array, got " + datum.print();
        return false;
    }
    set_out->clear();
    for (size_t i = 0; i < datum.arr_size(); ++i) {
        T value;
        if (!conv(datum.get(i), &value, error_out)) {
            return false;
        }
        auto res = set_out->insert(value);
        if (!allow_duplicates && !res.second) {
            *error_out = datum.get(i).print() + " was specified more than once.";
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
    bool init(ql::datum_t datum,
              std::string *error_out);
    bool get(const char *key,
             ql::datum_t *value_out,
             std::string *error_out);
    void get_optional(const char *key,
                      ql::datum_t *value_out);
    bool has(const char *key);
    bool check_no_extra_keys(std::string *error_out);
private:
    ql::datum_t datum;
    std::set<datum_string_t> extra_keys;
};

#endif /* CLUSTERING_ADMINISTRATION_DATUM_ADAPTER_HPP_ */

