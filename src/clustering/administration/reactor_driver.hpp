// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/semilattice/view.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

class perfmon_collection_repo_t;
// class serializer_t;
// class serializer_multiplexer_t;

template <class> class watchable_and_reactor_t;

template <class> class multistore_ptr_t;

// This type holds some protocol_t::store_t objects, and doesn't let anybody _casually_ touch them.
template <class protocol_t>
class stores_lifetimer_t {
public:
    stores_lifetimer_t() { }
    ~stores_lifetimer_t() {
        if (stores_.has()) {
            for (int i = 0, e = stores_.size(); i < e; ++i) {
                // TODO: This should use pmap.
                on_thread_t th(stores_[i]->home_thread());
                stores_[i].reset();
            }
        }
        if (serializer_.has()) {
            on_thread_t th(serializer_->home_thread());
            serializer_.reset();
            if (multiplexer_.has()) {
                multiplexer_.reset();
            }
        }
    }

    scoped_ptr_t<serializer_t> *serializer() { return &serializer_; }
    scoped_ptr_t<serializer_multiplexer_t> *multiplexer() { return &multiplexer_; }
    scoped_array_t<scoped_ptr_t<typename protocol_t::store_t> > *stores() { return &stores_; }

private:
    scoped_ptr_t<serializer_t> serializer_;
    scoped_ptr_t<serializer_multiplexer_t> multiplexer_;
    scoped_array_t<scoped_ptr_t<typename protocol_t::store_t> > stores_;

    DISABLE_COPYING(stores_lifetimer_t);
};

template <class protocol_t>
class svs_by_namespace_t {
public:
    virtual void get_svs(perfmon_collection_t *perfmon_collection, namespace_id_t namespace_id,
                         int64_t cache_size,
                         stores_lifetimer_t<protocol_t> *stores_out,
                         scoped_ptr_t<multistore_ptr_t<protocol_t> > *svs_out,
                         typename protocol_t::context_t *) = 0;
    virtual void destroy_svs(namespace_id_t namespace_id) = 0;

protected:
    virtual ~svs_by_namespace_t() { }
};

template <class> class ack_info_t;

template <class protocol_t>
class reactor_driver_t {
public:
    reactor_driver_t(const base_path_t &base_path,
                     io_backender_t *io_backender,
                     mailbox_manager_t *mbox_manager,
                     const clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > &directory_view,
                     branch_history_manager_t<protocol_t> *branch_history_manager,
                     boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > namespaces_view,
                     boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view,
                     const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_translation_table,
                     svs_by_namespace_t<protocol_t> *svs_by_namespace,
                     perfmon_collection_repo_t *,
                     typename protocol_t::context_t *);

    ~reactor_driver_t();

    clone_ptr_t<watchable_t<namespaces_directory_metadata_t<protocol_t> > > get_watchable() {
        return watchable_variable.get_watchable();
    }

private:
    template<class protocol2_t>
    friend class watchable_and_reactor_t;

    typedef boost::ptr_map<namespace_id_t, watchable_and_reactor_t<protocol_t> > reactor_map_t;

    void delete_reactor_data(
            auto_drainer_t::lock_t lock,
            typename reactor_map_t::auto_type *thing_to_delete,
            namespace_id_t namespace_id);
    void on_change();

    const base_path_t base_path;
    io_backender_t *const io_backender;
    mailbox_manager_t *const mbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > directory_view;
    branch_history_manager_t<protocol_t> *branch_history_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_translation_table;
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > namespaces_view;
    boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view;
    typename protocol_t::context_t *ctx;
    svs_by_namespace_t<protocol_t> *const svs_by_namespace;

    scoped_ptr_t<ack_info_t<protocol_t> > ack_info;

    watchable_variable_t<namespaces_directory_metadata_t<protocol_t> > watchable_variable;
    mutex_assertion_t watchable_variable_lock;

    boost::ptr_map<namespace_id_t, watchable_and_reactor_t<protocol_t> > reactor_data;

    auto_drainer_t drainer;

    typename semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > >::subscription_t semilattice_subscription;
    watchable_t<std::map<peer_id_t, machine_id_t> >::subscription_t translation_table_subscription;

    perfmon_collection_repo_t *perfmon_collection_repo;
    //boost::ptr_vector<perfmon_collection_t> namespace_perfmon_collections;
    DISABLE_COPYING(reactor_driver_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_ */
