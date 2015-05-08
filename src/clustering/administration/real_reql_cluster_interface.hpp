// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_REAL_REQL_CLUSTER_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_REAL_REQL_CLUSTER_INTERFACE_HPP_

#include <set>
#include <string>

#include "clustering/administration/admin_op_exc.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/tables/generate_config.hpp"
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
            rdb_context_t *rdb_context,
            server_config_client_t *server_config_client,
            table_meta_client_t *table_meta_client,
            watchable_map_t<
                std::pair<peer_id_t, std::pair<namespace_id_t, branch_id_t> >,
                table_query_bcard_t> *table_query_directory
            );

    bool db_create(const name_string_t &name,
            signal_t *interruptor, ql::datum_t *result_out, std::string *error_out);
    bool db_drop(const name_string_t &name,
            signal_t *interruptor, ql::datum_t *result_out, std::string *error_out);
    bool db_list(
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out);
    bool db_find(const name_string_t &name,
            signal_t *interruptor,
            counted_t<const ql::db_t> *db_out, std::string *error_out);
    bool db_config(
            const counted_t<const ql::db_t> &db,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            std::string *error_out);

    bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
            const table_generate_config_params_t &config_params,
            const std::string &primary_key, write_durability_t durability,
            signal_t *interruptor, ql::datum_t *result_out, std::string *error_out);
    bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, ql::datum_t *result_out, std::string *error_out);
    bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out);
    bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            boost::optional<admin_identifier_format_t> identifier_format,
            signal_t *interruptor, counted_t<base_table_t> *table_out,
            std::string *error_out);
    bool table_estimate_doc_counts(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::env_t *env,
            std::vector<int64_t> *doc_counts_out,
            std::string *error_out);
    bool table_config(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            std::string *error_out);
    bool table_status(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out,
            std::string *error_out);

    bool table_wait(
            counted_t<const ql::db_t> db,
            const name_string_t &name,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
            std::string *error_out);
    bool db_wait(
            counted_t<const ql::db_t> db,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
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

    bool sindex_create(
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            const sindex_config_t &config,
            signal_t *interruptor,
            std::string *error_out);
    bool sindex_drop(
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            signal_t *interruptor,
            std::string *error_out);
    bool sindex_rename(
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            const std::string &name,
            const std::string &new_name,
            bool overwrite,
            signal_t *interruptor,
            std::string *error_out);
    bool sindex_list(
            counted_t<const ql::db_t> db,
            const name_string_t &table,
            signal_t *interruptor,
            std::string *error_out,
            std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
                *configs_and_statuses_out);

    /* `calculate_split_points_with_distribution` needs access to the underlying
    `namespace_interface_t` and `table_meta_client_t`. */
    table_meta_client_t *get_table_meta_client() {
        return table_meta_client;
    }
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
    table_meta_client_t *table_meta_client;
    scoped_array_t< scoped_ptr_t< cross_thread_watchable_variable_t<
        databases_semilattice_metadata_t > > > cross_thread_database_watchables;
    rdb_context_t *rdb_context;

    namespace_repo_t namespace_repo;
    ql::changefeed::client_t changefeed_client;
    server_config_client_t *server_config_client;

    void wait_for_metadata_to_propagate(const cluster_semilattice_metadata_t &metadata,
                                        signal_t *interruptor);

    // This could soooo be optimized if you don't want to copy the whole thing.
    void get_databases_metadata(databases_semilattice_metadata_t *out);

    void make_single_selection(
            artificial_table_backend_t *table_backend,
            const name_string_t &table_name,
            const uuid_u &primary_key,
            ql::backtrace_id_t bt,
            ql::env_t *env,
            scoped_ptr_t<ql::val_t> *selection_out)
            THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, admin_op_exc_t);

    void wait_internal(
            std::set<namespace_id_t> tables,
            table_readiness_t readiness,
            signal_t *interruptor,
            ql::datum_t *result_out,
            int *count_out)
            THROWS_ONLY(interrupted_exc_t, admin_op_exc_t);

    void reconfigure_internal(
            const counted_t<const ql::db_t> &db,
            const namespace_id_t &table_id,
            const table_generate_config_params_t &params,
            bool dry_run,
            signal_t *interruptor,
            ql::datum_t *result_out)
            THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t,
                failed_table_op_exc_t, maybe_failed_table_op_exc_t, admin_op_exc_t);

    void rebalance_internal(
            const namespace_id_t &table_id,
            signal_t *interruptor,
            ql::datum_t *results_out)
            THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t,
                failed_table_op_exc_t, maybe_failed_table_op_exc_t, admin_op_exc_t);

    void sindex_change_internal(
            const counted_t<const ql::db_t> &db,
            const name_string_t &table_name,
            const std::function<void(std::map<std::string, sindex_config_t> *)> &cb,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t,
                failed_table_op_exc_t, maybe_failed_table_op_exc_t);

    DISABLE_COPYING(real_reql_cluster_interface_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REAL_REQL_CLUSTER_INTERFACE_HPP_ */

