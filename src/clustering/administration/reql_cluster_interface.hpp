// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_REQL_CLUSTER_INTERFACE_HPP_
#define CLUSTERING_ADMINISTRATION_REQL_CLUSTER_INTERFACE_HPP_

#include <set>
#include <string>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/context.hpp"
#include "rpc/semilattice/view.hpp"

/* `real_reql_cluster_interface_t` is a concrete subclass of `reql_cluster_interface_t`
that translates the user's `table_create()`, `table_drop()`, etc. requests into specific
actions on the semilattices. By performing these actions through the abstract
`reql_cluster_interface_t`, we can keep the ReQL code separate from the semilattice code.
*/

class real_reql_cluster_interface_t : public reql_cluster_interface_t {
public:
    real_reql_cluster_interface_t(
            mailbox_manager_t *mailbox_manager,
            machine_id_t my_machine_id,
            boost::shared_ptr< semilattice_readwrite_view_t<
                cluster_semilattice_metadata_t> > semilattices,
            clone_ptr_t< watchable_t< change_tracking_map_t<
                peer_id_t, cluster_directory_metadata_t> > > directory,
            rdb_context_t *rdb_context
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

    /* `distribution_app_t` needs access to the underlying `namespace_interface_t` */
    namespace_repo_t *get_namespace_repo() {
        return &namespace_repo;
    }

private:
    mailbox_manager_t *mailbox_manager;
    machine_id_t my_machine_id;
    boost::shared_ptr< semilattice_readwrite_view_t<
        cluster_semilattice_metadata_t> > semilattice_root_view;
    clone_ptr_t< watchable_t< change_tracking_map_t<
        peer_id_t, cluster_directory_metadata_t> > > directory_root_view;
    scoped_array_t< scoped_ptr_t< cross_thread_watchable_variable_t< cow_ptr_t<
        namespaces_semilattice_metadata_t> > > > cross_thread_namespace_watchables;
    scoped_array_t< scoped_ptr_t< cross_thread_watchable_variable_t<
        databases_semilattice_metadata_t > > > cross_thread_database_watchables;
    rdb_context_t *rdb_context;

    namespace_repo_t namespace_repo;
    ql::changefeed::client_t changefeed_client;

    void wait_for_metadata_to_propagate(const cluster_semilattice_metadata_t &metadata,
                                        signal_t *interruptor);

    cow_ptr_t<namespaces_semilattice_metadata_t> get_namespaces_metadata();
    // This could soooo be optimized if you don't want to copy the whole thing.
    void get_databases_metadata(databases_semilattice_metadata_t *out);

    DISABLE_COPYING(real_reql_cluster_interface_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REQL_CLUSTER_INTERFACE_HPP_ */

