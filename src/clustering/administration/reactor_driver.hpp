// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/servers/machine_id_to_peer_id.hpp"
#include "clustering/administration/servers/name_client.hpp"
#include "clustering/immediate_consistency/branch/backfill_throttler.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/semilattice/view.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

class io_backender_t;
class perfmon_collection_repo_t;
// class serializer_t;
// class serializer_multiplexer_t;

class watchable_and_reactor_t;

class multistore_ptr_t;

class rdb_context_t;

// This type holds some store_t objects, and doesn't let anybody _casually_ touch them.
class stores_lifetimer_t {
public:
    stores_lifetimer_t();
    ~stores_lifetimer_t();

    scoped_ptr_t<serializer_t> *serializer() { return &serializer_; }
    scoped_ptr_t<serializer_multiplexer_t> *multiplexer() { return &multiplexer_; }
    scoped_array_t<scoped_ptr_t<store_t> > *stores() { return &stores_; }

private:
    scoped_ptr_t<serializer_t> serializer_;
    scoped_ptr_t<serializer_multiplexer_t> multiplexer_;
    scoped_array_t<scoped_ptr_t<store_t> > stores_;

    DISABLE_COPYING(stores_lifetimer_t);
};

class svs_by_namespace_t {
public:
    virtual void get_svs(perfmon_collection_t *perfmon_collection, namespace_id_t namespace_id,
                         stores_lifetimer_t *stores_out,
                         scoped_ptr_t<multistore_ptr_t> *svs_out,
                         rdb_context_t *) = 0;
    virtual void destroy_svs(namespace_id_t namespace_id) = 0;

protected:
    virtual ~svs_by_namespace_t() { }
};

class ack_info_t;

class reactor_driver_t {
public:
    reactor_driver_t(
        const base_path_t &base_path,
         io_backender_t *io_backender,
         mailbox_manager_t *mbox_manager,
         watchable_map_t<
            std::pair<peer_id_t, namespace_id_t>,
            namespace_directory_metadata_t
            > *directory_view,
         branch_history_manager_t *branch_history_manager,
         boost::shared_ptr< semilattice_readwrite_view_t< cow_ptr_t<
            namespaces_semilattice_metadata_t> > > namespaces_view,
         server_name_client_t *server_name_client,
         signal_t *we_were_permanently_removed,
         svs_by_namespace_t *svs_by_namespace,
         perfmon_collection_repo_t *,
         rdb_context_t *);

    ~reactor_driver_t();

    watchable_map_t<namespace_id_t, namespace_directory_metadata_t> *get_watchable() {
        return &watchable_var;
    }

private:
    friend class watchable_and_reactor_t;

    typedef std::map<namespace_id_t, scoped_ptr_t<watchable_and_reactor_t> > reactor_map_t;
    typedef directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> >
        reactor_directory_entry_t;

    void delete_reactor_data(
            auto_drainer_t::lock_t lock,
            watchable_and_reactor_t *thing_to_delete,
            namespace_id_t namespace_id);
    void on_change();

    const base_path_t base_path;
    io_backender_t *const io_backender;
    mailbox_manager_t *const mbox_manager;
    watchable_map_t<
        std::pair<peer_id_t, namespace_id_t>,
        namespace_directory_metadata_t
        > *directory_view;
    branch_history_manager_t *branch_history_manager;
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > namespaces_view;
    server_name_client_t *server_name_client;
    signal_t *we_were_permanently_removed;
    rdb_context_t *ctx;
    svs_by_namespace_t *const svs_by_namespace;
    backfill_throttler_t backfill_throttler;

    watchable_map_var_t<namespace_id_t, namespace_directory_metadata_t> watchable_var;

    reactor_map_t reactor_data;

    auto_drainer_t drainer;

    semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> >
        ::subscription_t semilattice_subscription;
    watchable_t< std::multimap<name_string_t, machine_id_t> >::subscription_t
        name_to_machine_id_subscription;
    watchable_t< std::map<machine_id_t, peer_id_t> >::subscription_t
        machine_id_to_peer_id_subscription;

    perfmon_collection_repo_t *perfmon_collection_repo;
    //boost::ptr_vector<perfmon_collection_t> namespace_perfmon_collections;
    DISABLE_COPYING(reactor_driver_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_ */
