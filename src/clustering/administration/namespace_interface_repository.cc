#include "clustering/administration/namespace_interface_repository.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

template <class protocol_t>
namespace_repo_t<protocol_t>::namespace_repo_t(mailbox_manager_t *_mailbox_manager,
                                               clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > _namespaces_directory_metadata)
    : mailbox_manager(_mailbox_manager),
      namespaces_directory_metadata(_namespaces_directory_metadata)
{ }

template <class protocol_t>
std::map<peer_id_t, reactor_business_card_t<protocol_t> > get_reactor_business_cards(
        const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &ns_directory_metadata, namespace_id_t n_id) {
    std::map<peer_id_t, reactor_business_card_t<protocol_t> > res;
    for (typename std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> >::const_iterator it  = ns_directory_metadata.begin();
                                                                                                    it != ns_directory_metadata.end();
                                                                                                    it++) {
        typename namespaces_directory_metadata_t<protocol_t>::reactor_bcards_map_t::const_iterator jt =
            it->second.reactor_bcards.find(n_id);
        if (jt != it->second.reactor_bcards.end()) {
            res[it->first] = jt->second.internal;
        } else {
            res[it->first] = reactor_business_card_t<protocol_t>();
        }
    }

    return res;
}

template<class protocol_t>
namespace_repo_t<protocol_t>::access_t::access_t(namespace_repo_t *parent, namespace_id_t ns_id, signal_t *interruptor) {
    typename boost::ptr_map<namespace_id_t, cluster_namespace_interface_t<protocol_t> >::iterator it =
        parent->interface_map.find(ns_id);
    if (it == parent->interface_map.end()) {
        ns_if = new cluster_namespace_interface_t<protocol_t>(parent->mailbox_manager,
            parent->namespaces_directory_metadata->
                subview(boost::bind(&get_reactor_business_cards<protocol_t>, _1, ns_id))
            );
        parent->interface_map.insert(ns_id, ns_if);
    } else {
        ns_if = it->second;
    }
    wait_interruptible(ns_if->get_initial_ready_signal(), interruptor);
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"

template class namespace_repo_t<mock::dummy_protocol_t>;
template class namespace_repo_t<memcached_protocol_t>;
