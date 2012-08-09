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
namespace_repo_t<protocol_t>::namespace_data_t::~namespace_data_t() {
   for (int i = 0; (size_t)i < thread_data.size(); ++i) {
        if (thread_data[i].ns_if != NULL) {
            on_thread_t rethreader(i);
            delete thread_data[i].ns_if;
            delete thread_data[i].ready;
            delete thread_data[i].deleter;
        }
        delete thread_data[i].ct_subview;
    }
}

template<class protocol_t>
namespace_repo_t<protocol_t>::access_t::ref_handler_t::ref_handler_t(typename namespace_data_t::thread_data_t *_thread_data) :
    thread_data(_thread_data)
{
    ++thread_data->refs;
}

template<class protocol_t>
namespace_repo_t<protocol_t>::access_t::ref_handler_t::~ref_handler_t() {
    --thread_data->refs;
    if (thread_data->refs == 0) {
        rassert(thread_data->deleter == NULL);
        thread_data->deleter = new typename namespace_data_t::timed_delete_t(thread_data);
    }
}

template<class protocol_t>
namespace_repo_t<protocol_t>::access_t::access_t(namespace_repo_t *_parent, namespace_id_t _ns_id, signal_t *interruptor) :
    thread_data(NULL),
    parent(_parent),
    ns_id(_ns_id),
    thread(get_thread_id())
{
    typename std::map<namespace_id_t, namespace_data_t>::iterator it = parent->interface_map.find(ns_id);
    if (it == parent->interface_map.end()) {
        it = parent->interface_map.insert(std::make_pair(ns_id, namespace_data_t())).first;
    }
    thread_data = &it->second.thread_data[thread];

    // If there is a pending delete for the ns_if, cancel it
    if (thread_data->refs == 0 && thread_data->deleter != NULL) {
        delete thread_data->deleter;
        thread_data->deleter = NULL;
    }
    ref_handler.init(new ref_handler_t(thread_data));

    // Check if the ns_if and supporting members have been constructed
    if (thread_data->ready == NULL) {
        thread_data->ready = new cond_t();
        rassert(thread_data->ns_if == NULL);
        rassert(thread_data->ct_subview == NULL);

        // Watchables must be constructed on the original thread
        {
            on_thread_t rethreader(parent->home_thread());
            clone_ptr_t<watchable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > > > subview =
                parent->namespaces_directory_metadata->subview(boost::bind(&get_reactor_business_cards<protocol_t>, _1, ns_id));
            thread_data->ct_subview =
                new cross_thread_watchable_variable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > >(subview, thread);
        }

        // Namespace interface created on access's thread
        thread_data->ns_if = new cluster_namespace_interface_t<protocol_t>(parent->mailbox_manager,
                                                                           thread_data->ct_subview->get_watchable());
        thread_data->ready->pulse();
    } else if (!thread_data->ready->is_pulsed()) {
        wait_interruptible(thread_data->ready, interruptor);
    }

    // Make sure the ns_if is ready
    if (!thread_data->ns_if->get_initial_ready_signal()->is_pulsed()) {
        wait_interruptible(thread_data->ns_if->get_initial_ready_signal(), interruptor);
    }

    // Sanity check to make sure no one's trying to delete our ns_if
    rassert(thread_data->deleter == NULL);
}

template <class protocol_t>
namespace_repo_t<protocol_t>::access_t::access_t(const access_t& access) :
    thread_data(access.thread_data),
    parent(access.parent),
    ns_id(access.ns_id),
    thread(access.thread)
{
    // Increment thread_data's refs, and make sure that it's not under a pending delete
    //  which should be impossible, because there's an existing access already
    ref_handler.init(new ref_handler_t(thread_data));
    rassert(thread_data->deleter == NULL);
    rassert(thread_data->ready != NULL);
    rassert(thread_data->ready->is_pulsed());
    rassert(thread_data->ns_if->get_initial_ready_signal()->is_pulsed());
}

template <class protocol_t>
namespace_repo_t<protocol_t>::access_t::~access_t() {
    rassert(thread == get_thread_id());
    rassert(thread_data != NULL);
}

template <class protocol_t>
cluster_namespace_interface_t<protocol_t> *namespace_repo_t<protocol_t>::access_t::get_namespace_if() {
    rassert(thread == get_thread_id());
    rassert(thread_data != NULL);
    rassert(thread_data->ns_if != NULL);
    return thread_data->ns_if;
}

template <class protocol_t>
namespace_repo_t<protocol_t>::namespace_data_t::timed_delete_t::timed_delete_t(thread_data_t *thread_data)
{
   // Spawn coroutine to delete after time has passed
   coro_t::spawn_sometime(boost::bind(&namespace_repo_t<protocol_t>::namespace_data_t::timed_delete_t::wait_and_delete,
                                          this, thread_data, auto_drainer_t::lock_t(&drainer)));
}

template <class protocol_t>
void namespace_repo_t<protocol_t>::namespace_data_t::timed_delete_t::wait_and_delete(thread_data_t *thread_data, auto_drainer_t::lock_t keepalive) {
    signal_timer_t waiter(timeout_ms);
    try {
        wait_interruptible(&waiter, keepalive.get_drain_signal());

        rassert(thread_data->refs == 0);
        rassert(thread_data->deleter == this);

        // In case anything blocks, get the thread_data back into its null-state before doing any deletes
        cond_t *old_ready = thread_data->ready;
        cluster_namespace_interface_t<protocol_t>* old_ns_if = thread_data->ns_if;
        cross_thread_watchable_variable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > >* old_ct_subview = thread_data->ct_subview;

        thread_data->ready = NULL;
        thread_data->ns_if = NULL;
        thread_data->ct_subview = NULL;
        thread_data->deleter = NULL;

        delete old_ns_if;
        delete old_ready;

        if (old_ct_subview != NULL) {
            on_thread_t rethreader(old_ct_subview->home_thread());
            delete old_ct_subview;
        }

        // Release our lock so we can destroy the deleter (the cond_t is no longer needed)
        keepalive.reset();
        delete this; // TODO: yeah, this is ugly, but it should be fine, we're done with 'this' at this point
    } catch (interrupted_exc_t &ex) {
        // We were deleted before the timer expired, ns_if is in use, so do nothing
    }
}

#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class namespace_repo_t<mock::dummy_protocol_t>;
template class namespace_repo_t<memcached_protocol_t>;
template class namespace_repo_t<rdb_protocol_t>;
