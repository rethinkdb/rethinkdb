// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_

#include <map>
#include <set>
#include <string>

#include "clustering/administration/cluster_config.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/server_config.hpp"
#include "clustering/administration/servers/server_status.hpp"
#include "clustering/administration/tables/db_config.hpp"
#include "clustering/administration/tables/table_config.hpp"
#include "clustering/administration/tables/table_status.hpp"
#include "clustering/administration/issues/issues_backend.hpp"
#include "containers/name_string.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/artificial_table/in_memory.hpp"
#include "rdb_protocol/context.hpp"

class real_reql_cluster_interface_t;
class server_name_client_t;

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
    bool table_config(const boost::optional<name_string_t> &name,
            counted_t<const ql::db_t> db, const ql::protob_t<const Backtrace> &bt,
            signal_t *interruptor, counted_t<ql::val_t> *resp_out,
            std::string *error_out);
    bool table_status(const boost::optional<name_string_t> &name,
            counted_t<const ql::db_t> db, const ql::protob_t<const Backtrace> &bt,
            signal_t *interruptor, counted_t<ql::val_t> *resp_out,
            std::string *error_out);

    bool table_reconfigure(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *new_config_out,
            std::string *error_out);

private:
    name_string_t database;
    std::map<name_string_t, artificial_table_backend_t *> tables;
    reql_cluster_interface_t *next;
};

/* `admin_artificial_tables_t` constructs the `artificial_reql_cluster_interface_t` along
with all of the tables that will go in it. */

class admin_artificial_tables_t {
public:
    admin_artificial_tables_t(
            real_reql_cluster_interface_t *_next_reql_cluster_interface,
            boost::shared_ptr< semilattice_readwrite_view_t<
                cluster_semilattice_metadata_t> > _semilattice_view,
            boost::shared_ptr< semilattice_readwrite_view_t<
                auth_semilattice_metadata_t> > _auth_view,
            clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
                cluster_directory_metadata_t> > > _directory_view,
            watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                            namespace_directory_metadata_t> *_reactor_directory_view,
            server_name_client_t *_name_client);
    reql_cluster_interface_t *get_reql_cluster_interface() {
        return reql_cluster_interface.get();
    }

    scoped_ptr_t<in_memory_artificial_table_backend_t> debug_scratch_backend;

    scoped_ptr_t<cluster_config_artificial_table_backend_t> cluster_config_backend;
    scoped_ptr_t<db_config_artificial_table_backend_t> db_config_backend;
    scoped_ptr_t<issues_artificial_table_backend_t> issues_backend;
    scoped_ptr_t<server_config_artificial_table_backend_t> server_config_backend;
    scoped_ptr_t<server_status_artificial_table_backend_t> server_status_backend;
    scoped_ptr_t<table_config_artificial_table_backend_t> table_config_backend;
    scoped_ptr_t<table_status_artificial_table_backend_t> table_status_backend;
    scoped_ptr_t<artificial_reql_cluster_interface_t> reql_cluster_interface;
};

#endif /* CLUSTERING_ADMINISTRATION_ARTIFICIAL_REQL_CLUSTER_INTERFACE_HPP_ */

