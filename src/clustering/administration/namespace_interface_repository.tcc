#ifndef __CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_RESPOSITORY_TCC__
#define __CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_RESPOSITORY_TCC__

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"

template <class protocol_t>
namespace_registry_t<protocol_t>::namespace_registry_t(mailbox_manager_t *_mailbox_manager, 
                                                       boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<protocol_t> > > _namespaces_semilattice_metadata,
                                                       clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > _namespaces_directory_metadata)
    : mailbox_manager(_mailbox_manager), 
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      namespaces_directory_metadata(_namespaces_directory_metadata)
      //namespaces_subscription(boost::bind(&namespace_registry_t<protocol_t>::on_change, this), namespaces_semilattice_metadata)
{ }

template <class protocol_t>
std::map<peer_id_t, typename namespace_registry_t<protocol_t>::master_map_t> get_master_map(const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &ns_directory_metadata, namespace_id_t n_id) {
    std::map<peer_id_t, typename namespace_registry_t<protocol_t>::master_map_t> res;
    for (typename std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> >::const_iterator it  = ns_directory_metadata.begin();
                                                                                                    it != ns_directory_metadata.end();
                                                                                                    it++) {
        if (std_contains(it->second.master_maps, n_id)) {
            res[it->first] = it->second.master_maps.find(n_id)->second;
        } else {
            res[it->first] = typename namespace_registry_t<protocol_t>::master_map_t();
        }
    }

    return res;
}

template <class protocol_t>
typename namespace_registry_t<protocol_t>::namepsace_if_access_t namespace_registry_t<protocol_t>::get_namespace_if(namespace_id_t n_id) {
    if (!std_contains(interface_map, n_id)) {
        interface_map.insert(n_id, new namespace_if_and_drainer_t(mailbox_manager,
                                                                 namespaces_directory_metadata->
                                                                    subview(boost::bind(&get_master_map<protocol_t>, _1, n_id))));
    }
    return namepsace_if_access_t(&interface_map.find(n_id)->second->interface, auto_drainer_t::lock_t(&interface_map.find(n_id)->second->drainer));
}

/* template <class protocol_t>
void namespace_registry_t<protocol_t>::on_change() {
        namespaces_semilattice_metadata_t<protocol_t> snapshot = namespaces_semilattice_metadata->get();

        for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator it  = snapshot.namespaces.begin();
                                                                                               it != snapshot.namespaces.end();
                                                                                               ++it) {

            if (!std_contains(interface_map, it->first) && !it->second.is_deleted()) {
                namespace_id_t tmp = it->first;
                interface_map.insert(tmp, new namespace_if_and_drainer_t(mailbox_manager,
                                                                         namespaces_directory_metadata->
                                                                            subview(boost::bind(&get_master_map<protocol_t>, _1, it->first))));
            } else if (std_contains(interface_map, it->first) && it->second.is_deleted()) {
                //The namespace has been deleted... get rid of the parser
                coro_t::spawn_sometime(boost::bind(&delete_a_thing<typename namespace_registry_t<protocol_t>::interface_map_t::auto_type>, new typename namespace_registry_t<protocol_t>::interface_map_t::auto_type(interface_map.release(interface_map.find(it->first)))));
            } else {
                //do nothing
            }
        }

    } */

#endif
