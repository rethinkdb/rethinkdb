#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/view.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

class perfmon_collection_repo_t;

template <class> class watchable_and_reactor_t;

template <class> class multistore_ptr_t;

template <class protocol_t>
class svs_by_namespace_t {
public:
    virtual void get_svs(perfmon_collection_t *perfmon_collection, namespace_id_t namespace_id,
                         boost::scoped_array<boost::scoped_ptr<typename protocol_t::store_t> > *stores_out,
                         boost::scoped_ptr<multistore_ptr_t<protocol_t> > *svs_out) = 0;
    virtual void destroy_svs(namespace_id_t namespace_id) = 0;

protected:
    virtual ~svs_by_namespace_t() { }
};

template <class protocol_t>
class reactor_driver_t {
public:
    reactor_driver_t(mailbox_manager_t *_mbox_manager,
                     const clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > &_directory_view,
                     boost::shared_ptr<semilattice_readwrite_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_view,
                     boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view_,
                     const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &_machine_id_translation_table,
                     svs_by_namespace_t<protocol_t> *_svs_by_namespace,
                     perfmon_collection_repo_t *);

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

    mailbox_manager_t *mbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > directory_view;
    boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_translation_table;
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > namespaces_view;
    boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view;
    svs_by_namespace_t<protocol_t> *const svs_by_namespace;

    watchable_variable_t<namespaces_directory_metadata_t<protocol_t> > watchable_variable;
    mutex_assertion_t watchable_variable_lock;

    boost::ptr_map<namespace_id_t, watchable_and_reactor_t<protocol_t> > reactor_data;

    auto_drainer_t drainer;

    typename semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> >::subscription_t semilattice_subscription;
    watchable_t<std::map<peer_id_t, machine_id_t> >::subscription_t translation_table_subscription;

    perfmon_collection_repo_t *perfmon_collection_repo;
    //boost::ptr_vector<perfmon_collection_t> namespace_perfmon_collections;
    DISABLE_COPYING(reactor_driver_t);
};

#endif /* CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_ */
