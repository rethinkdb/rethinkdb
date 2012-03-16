#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/directory/view.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/view.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

template<class protocol_t>
class watchable_and_reactor_t;

template <class protocol_t>
class reactor_driver_t {
public:
    reactor_driver_t(mailbox_manager_t *_mbox_manager,
                     clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > _directory_view,
                     boost::shared_ptr<semilattice_readwrite_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_view,
                     const clone_ptr_t<directory_rview_t<machine_id_t> > &_machine_id_translation_table,
                     std::string _file_path);

    ~reactor_driver_t();

private:
    typedef boost::ptr_map<namespace_id_t, watchable_and_reactor_t<protocol_t> > reactor_map_t;
 
    void delete_reactor_data(auto_drainer_t::lock_t lock, typename reactor_map_t::auto_type *thing_to_delete);
    void on_change();

    mailbox_manager_t *mbox_manager;
    clone_ptr_t<directory_rwview_t<namespaces_directory_metadata_t<protocol_t> > > directory_view;
    boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;
    clone_ptr_t<directory_rview_t<machine_id_t> > machine_id_translation_table;
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > namespaces_view;
    std::string file_path;

    boost::ptr_map<namespace_id_t, watchable_and_reactor_t<protocol_t> > reactor_data;

    auto_drainer_t drainer;

    typename semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> >::subscription_t semilattice_subscription;
    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
};

#endif
