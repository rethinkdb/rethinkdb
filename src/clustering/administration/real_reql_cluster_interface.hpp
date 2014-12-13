// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_REAL_REQL_CLUSTER_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_REAL_REQL_CLUSTER_INTERFACE_HPP_

#include <set>
#include <string>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/context.hpp"
#include "rpc/semilattice/view.hpp"

class admin_artificial_tables_t;
class artificial_table_backend_t;
class server_config_client_t;

/* `real_reql_cluster_interface_t` is a concrete subclass of `reql_cluster_interface_t`
that translates the user's `table_create()`, `table_drop()`, etc. requests into specific
actions on the semilattices. By performing these actions through the abstract
`reql_cluster_interface_t`, we can keep the ReQL code separate from the semilattice code.
*/

class real_reql_cluster_interface_t : public reql_cluster_interface_t {
public:
    real_reql_cluster_interface_t(
            mailbox_manager_t *mailbox_manager,
            boost::shared_ptr< semilattice_readwrite_view_t<
                cluster_semilattice_metadata_t> > semilattices,
            watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                namespace_directory_metadata_t> *directory_root_view,
            rdb_context_t *rdb_context,
            server_config_client_t *server_config_client
            );

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
    bool db_config(const std::vector<name_string_t> &db_names,
            const ql::protob_t<const Backtrace> &bt,
            signal_t *interruptor, scoped_ptr_t<ql::val_t> *resp_out,
            std::string *error_out);

    bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
            const table_generate_config_params_t &config_params,
            const std::string &primary_key, signal_t *interruptor,
            std::string *error_out);
    bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, std::string *error_out);
    bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out);
    bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            boost::optional<admin_identifier_format_t> identifier_format,
            signal_t *interruptor, counted_t<base_table_t> *table_out,
            std::string *error_out);
    bool table_config(counted_t<const ql::db_t> db,
            const std::vector<name_string_t> &tables,
            const ql::protob_t<const Backtrace> &bt,
            signal_t *interruptor, scoped_ptr_t<ql::val_t> *resp_out,
            std::string *error_out);
    bool table_status(counted_t<const ql::db_t> db,
            const std::vector<name_string_t> &tables,
            const ql::protob_t<const Backtrace> &bt,
            signal_t *interruptor, scoped_ptr_t<ql::val_t> *resp_out,
            std::string *error_out);
    bool table_wait(counted_t<const ql::db_t> db,
            const std::vector<name_string_t> &tables,
            table_readiness_t readiness,
            const ql::protob_t<const Backtrace> &bt,
            signal_t *interruptor, scoped_ptr_t<ql::val_t> *resp_out,
            std::string *error_out);

    bool table_reconfigure(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            std::string *error_out);
    bool db_reconfigure(
            counted_t<const ql::db_t> db,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            std::string *error_out);
    bool table_rebalance(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            signal_t *interruptor,
            ql::datum_t *result_out,
            std::string *error_out);
    bool db_rebalance(
            counted_t<const ql::db_t> db,
            signal_t *interruptor,
            ql::datum_t *result_out,
            std::string *error_out);
    bool table_estimate_doc_counts(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::env_t *env,
            std::vector<int64_t> *doc_counts_out,
            std::string *error_out);

    /* `calculate_split_points_with_distribution` needs access to the underlying
    `namespace_interface_t` */
    namespace_repo_t *get_namespace_repo() {
        return &namespace_repo;
    }

    /* This is public because it needs to be set after we're created to solve a certain
    chicken-and-egg problem */
    admin_artificial_tables_t *admin_tables;

private:
    mailbox_manager_t *mailbox_manager;
    boost::shared_ptr< semilattice_readwrite_view_t<
        cluster_semilattice_metadata_t> > semilattice_root_view;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                    namespace_directory_metadata_t> *directory_root_view;
    scoped_array_t< scoped_ptr_t< cross_thread_watchable_variable_t< cow_ptr_t<
        namespaces_semilattice_metadata_t> > > > cross_thread_namespace_watchables;
    scoped_array_t< scoped_ptr_t< cross_thread_watchable_variable_t<
        databases_semilattice_metadata_t > > > cross_thread_database_watchables;
    rdb_context_t *rdb_context;

    namespace_repo_t namespace_repo;
    ql::changefeed::client_t changefeed_client;
    server_config_client_t *server_config_client;

    void wait_for_metadata_to_propagate(const cluster_semilattice_metadata_t &metadata,
                                        signal_t *interruptor);

    cow_ptr_t<namespaces_semilattice_metadata_t> get_namespaces_metadata();
    // This could soooo be optimized if you don't want to copy the whole thing.
    void get_databases_metadata(databases_semilattice_metadata_t *out);

    bool get_table_ids_for_query(
            const counted_t<const ql::db_t> &db,
            const std::vector<name_string_t> &table_names,
            std::vector<std::pair<namespace_id_t, name_string_t> > *tables_out,
            std::string *error_out);

    /* For each UUID in `tables`, reads the row with that primary key from `backend`, and
    returns a vector of all the rows. If `error_on_missing` is false, missing rows will
    be silently ignored; otherwise, an error will be raised. */
    bool table_meta_read(artificial_table_backend_t *backend,
            const counted_t<const ql::db_t> &db,
            const std::vector<std::pair<namespace_id_t, name_string_t> > &tables,
            bool error_on_missing,
            signal_t *interruptor,
            std::vector<ql::datum_t> *res_out,
            std::string *error_out);

    bool reconfigure_internal(
            const counted_t<const ql::db_t> &db,
            const namespace_id_t &table_id,
            const name_string_t &table_name,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out,
            std::string *error_out);
    bool rebalance_internal(
            const counted_t<const ql::db_t> &db,
            const namespace_id_t &table_id,
            const name_string_t &table_name,
            signal_t *interruptor,
            ql::datum_t *results_out,
            std::string *error_out);

    DISABLE_COPYING(real_reql_cluster_interface_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REAL_REQL_CLUSTER_INTERFACE_HPP_ */

