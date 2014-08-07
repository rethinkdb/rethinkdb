// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_

#include <map>
#include <set>
#include <string>

#include "containers/name_string.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/context.hpp"

/* The `artificial_reql_cluster_interface_t` is responsible for handling queries to the
`rethinkdb` database. It's implemented as a proxy over the
`real_reql_cluster_interface_t`; queries go first to the `artificial_...`, and if they
aren't related to the `rethinkdb` database, they get passed on to the `real_...`. */

class artificial_reql_cluster_interface_t : public reql_cluster_interface_t {
public:
    artificial_reql_cluster_interface_t(
            /* This is the name of the special database; i.e. `rethinkdb` */
            name_string_t _database,
            /* These are the tables that live in the special database. */
            const std::map<name_string_t, artificial_table_backend_t *> &_tables,
            /* This is the `real_reql_cluster_interface_t` that we're proxying. */
            reql_cluster_interface_t *_next) :
        database(_database),
        tables(_tables),
        next(_next) { }

    bool db_create(const name_string_t &name,
            signal_t *interruptor, std::string *error_out);
    bool db_drop(const name_string_t &name,
            signal_t *interruptor, std::string *error_out);
    bool db_list(
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out);
    bool db_find(const name_string_t &name,
            signal_t *interruptor,
            counted_t<const ql::db_t> *db_out, std::string *error_out);

    bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
            const boost::optional<name_string_t> &primary_dc, bool hard_durability,
            const std::string &primary_key, signal_t *interruptor,
            std::string *error_out);
    bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, std::string *error_out);
    bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out);
    bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, scoped_ptr_t<base_table_t> *table_out,
            std::string *error_out);

    bool server_rename(const name_string_t &old_name, const name_string_t &new_name,
            signal_t *interruptor, std::string *error_out);

private:
    name_string_t database;
    std::map<name_string_t, artificial_table_backend_t *> tables;
    reql_cluster_interface_t *next;
};

#endif /* CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_ */

