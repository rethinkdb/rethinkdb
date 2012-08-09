#ifndef CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_
#define CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_

#include <map>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "concurrency/cross_thread_watchable.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/reactor/namespace_interface.hpp"

/* `namespace_repo_t` is responsible for constructing and caching
`cluster_namespace_interface_t` objects for all of the namespaces in the cluster
for a given protocol. Caching `cluster_namespace_interface_t` objects is
important because every time a new `cluster_namespace_interface_t` is created,
it must perform a handshake with every `master_t`, which means several network
round-trips. */

template <class protocol_t>
class namespace_repo_t : public home_thread_mixin_t {
    struct namespace_data_t {
        namespace_data_t() : thread_data(get_num_threads()) { }
        ~namespace_data_t();

        struct thread_data_t;

        class timed_delete_t {
        public:
           timed_delete_t(thread_data_t *thread_data);
        private:
           // After 20 seconds of inactivity, clean up the namespace interface for a thread
           const static int64_t timeout_ms = 20000;
           void wait_and_delete(thread_data_t *thread_data, auto_drainer_t::lock_t keepalive);

           auto_drainer_t drainer;
        };

        struct thread_data_t {
            thread_data_t() : ct_subview(NULL), ns_if(NULL), deleter(NULL), ready(NULL), refs(0) { }

            cross_thread_watchable_variable_t<std::map<peer_id_t, reactor_business_card_t<protocol_t> > >* ct_subview;
            cluster_namespace_interface_t<protocol_t>* ns_if;
            timed_delete_t *deleter;
            cond_t* ready;
            uint32_t refs;
        };

        std::vector<thread_data_t> thread_data;
    };

public:
    namespace_repo_t(mailbox_manager_t *,
                     clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > >);

    class access_t {
    public:
        access_t(namespace_repo_t *_parent, namespace_id_t _ns_id, signal_t *interruptor);
        access_t(const access_t& access);
        ~access_t();

        cluster_namespace_interface_t<protocol_t> *get_namespace_if();

    private:

        class ref_handler_t {
        public: 
            ref_handler_t(typename namespace_data_t::thread_data_t *_thread_data);
            ~ref_handler_t();
        private:
            typename namespace_data_t::thread_data_t *thread_data;
        };

        scoped_ptr_t<ref_handler_t> ref_handler;
        typename namespace_data_t::thread_data_t *thread_data;
        namespace_repo_t *parent;
        namespace_id_t ns_id;
        int thread;
    };

private:
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > namespaces_directory_metadata;

    std::map<namespace_id_t, namespace_data_t> interface_map;
    DISABLE_COPYING(namespace_repo_t);
};

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_ */
