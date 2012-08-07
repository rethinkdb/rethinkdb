#include "clustering/administration/namespace_interface_repository.hpp"
#include "concurrency/cross_thread_signal.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

template <class protocol_t>
namespace_repo_t<protocol_t>::namespace_repo_t(mailbox_manager_t *_mailbox_manager,
                                               clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > _namespaces_directory_metadata)
    : mailbox_manager(_mailbox_manager),
      namespaces_directory_metadata(_namespaces_directory_metadata)
{ }

/*
template <class protocol_t>
namespace_repo_t<protocol_t>::~namespace_repo_t() {
    std::map<namespace_id_t, std::vector<namespace_data_t> > interface_map;
    for (typename std::map<namespace_id_t, std::vector<namespace_data_t> >::iterator i = interface_map.begin(); i != interface_map.end(); ++i) {
    }
}*/

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

template <class protocol_t>
void namespace_repo_t<protocol_t>::access_t::initialize_namespace_interface(clone_ptr_t<watchable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > > > subview, mailbox_manager_t *mailbox_manager, int thread) {
    rassert(ns_data->thread_data[thread].ns_if == NULL);
    rassert(ns_data->thread_data[thread].ct_subview == NULL);

    // Cross thread watchable must be constructed on original thread
    ns_data->thread_data[thread].ct_subview =
        new cross_thread_watchable_variable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > >(subview, thread);
    
    // Namespace interface must be constructed on new thread
    {
        on_thread_t rethreader(thread);
        ns_data->thread_data[thread].ns_if =
            new cluster_namespace_interface_t<protocol_t>(mailbox_manager,
                                                          ns_data->thread_data[thread].ct_subview->get_watchable());
    }
}

template<class protocol_t>
namespace_repo_t<protocol_t>::access_t::access_t(namespace_repo_t *_parent, namespace_id_t _ns_id, signal_t *interruptor) :
    ns_data(NULL),
    parent(_parent),
    ns_id(_ns_id)
{
    parent->assert_thread();
    typename std::map<namespace_id_t, namespace_data_t>::iterator it = parent->interface_map.find(ns_id);
    if (it == parent->interface_map.end()) {
        ns_data = &parent->interface_map.insert(std::make_pair(ns_id, namespace_data_t())).first->second;

        clone_ptr_t<watchable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > > > subview =
            parent->namespaces_directory_metadata->subview(boost::bind(&get_reactor_business_cards<protocol_t>, _1, ns_id));

        // This could probably be done more in parallel, but this is done rather infrequently, so it should be fine
        for (int i = 0; i < get_num_threads(); ++i) {
            initialize_namespace_interface(subview, parent->mailbox_manager, i);
        }

        // Make sure all the interfaces are ready before returning
        for (int i = 0; i < get_num_threads(); ++i) {
            cross_thread_signal_t cross_thread_interruptor(interruptor, i);
            on_thread_t rethreader(i);
            wait_interruptible(ns_data->thread_data[i].ns_if->get_initial_ready_signal(), &cross_thread_interruptor);
        }

    } else {
        ns_data = &it->second;
    }
    ++ns_data->refs;
}

template <class protocol_t>
namespace_repo_t<protocol_t>::access_t::~access_t() {
    rassert(ns_data != NULL);
    parent->assert_thread();
    --ns_data->refs;

    if (ns_data->refs == 0) {
        parent->interface_map.erase(ns_id);
    }
}


template <class protocol_t>
cluster_namespace_interface_t<protocol_t> *namespace_repo_t<protocol_t>::access_t::get_namespace_if() {
    return ns_data->thread_data[get_thread_id()].ns_if;
}

#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class namespace_repo_t<mock::dummy_protocol_t>;
template class namespace_repo_t<memcached_protocol_t>;
template class namespace_repo_t<rdb_protocol_t>;
