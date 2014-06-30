// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
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
    reactor_driver_t(const base_path_t &base_path,
                     io_backender_t *io_backender,
                     mailbox_manager_t *mbox_manager,
                     const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> > > &directory_view,
                     branch_history_manager_t *branch_history_manager,
                     boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > namespaces_view,
                     boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view,
                     const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > &machine_id_translation_table,
                     svs_by_namespace_t *svs_by_namespace,
                     perfmon_collection_repo_t *,
                     rdb_context_t *);

    ~reactor_driver_t();

    clone_ptr_t<watchable_t<namespaces_directory_metadata_t> > get_watchable() {
        return watchable_variable.get_watchable();
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
    void set_reactor_directory_entry(
        const namespace_id_t reactor_namespace,
        const boost::optional<reactor_directory_entry_t> &new_value);
    void commit_directory_changes(auto_drainer_t::lock_t lock);
    // This function is passed by `commit_directory_changes()` into the
    // `apply_read()` method of the directory watchable
    static bool apply_directory_changes(
        std::map<namespace_id_t, boost::optional<reactor_directory_entry_t> >
            *_changed_reactor_directories,
        namespaces_directory_metadata_t *directory);

    const base_path_t base_path;
    io_backender_t *const io_backender;
    mailbox_manager_t *const mbox_manager;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> > > directory_view;
    branch_history_manager_t *branch_history_manager;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > machine_id_translation_table;
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > namespaces_view;
    boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view;
    rdb_context_t *ctx;
    svs_by_namespace_t *const svs_by_namespace;
    backfill_throttler_t backfill_throttler;

    scoped_ptr_t<ack_info_t> ack_info;

    watchable_variable_t<namespaces_directory_metadata_t> watchable_variable;
    mutex_assertion_t watchable_variable_lock;

    // `changed_reactor_directories` collects reactor directory entries that
    // have changed since the last execution of commit_directory_changes().
    // This is a performance optimization, as it allows us to aggregate
    // multiple directory changes into a single commit.
    // If the optional is none, that means that the entry should be deleted.
    std::map<namespace_id_t, boost::optional<reactor_directory_entry_t> >
        changed_reactor_directories;
    // We need a separate drainer for this because it must stay alive
    // until after reactor_data is destructed.
    auto_drainer_t directory_change_drainer;

    reactor_map_t reactor_data;

    auto_drainer_t drainer;

    semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> >::subscription_t semilattice_subscription;
    watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> >::subscription_t translation_table_subscription;

    perfmon_collection_repo_t *perfmon_collection_repo;
    //boost::ptr_vector<perfmon_collection_t> namespace_perfmon_collections;
    DISABLE_COPYING(reactor_driver_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_ */
