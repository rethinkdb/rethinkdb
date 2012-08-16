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
private:
    struct namespace_cache_entry_t;
    struct namespace_cache_t;

public:
    namespace_repo_t(mailbox_manager_t *,
                     clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > >,
                     typename protocol_t::context_t *);

    class access_t {
    public:
        access_t();
        access_t(namespace_repo_t *parent, namespace_id_t namespace_id, signal_t *interruptor);
        access_t(const access_t& access);
        access_t &operator=(const access_t &access);

        namespace_interface_t<protocol_t> *get_namespace_if();

    private:
        struct ref_handler_t {
        public:
            ref_handler_t();
            ~ref_handler_t();
            void init(namespace_cache_entry_t *_ref_target);
            void reset();
        private:
            namespace_cache_entry_t *ref_target;
        };
        namespace_cache_entry_t *cache_entry;
        ref_handler_t ref_handler;
        int thread;
    };

private:
    void create_and_destroy_namespace_interface(
            namespace_cache_t *cache,
            namespace_id_t namespace_id,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING;

    struct namespace_cache_entry_t {
    public:
        promise_t<namespace_interface_t<protocol_t> *> namespace_if;
        int ref_count;
        cond_t *pulse_when_ref_count_becomes_zero;
        cond_t *pulse_when_ref_count_becomes_nonzero;
    };

    struct namespace_cache_t {
    public:
        boost::ptr_map<namespace_id_t, namespace_cache_entry_t> entries;
        auto_drainer_t drainer;
    };

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > namespaces_directory_metadata;
    typename protocol_t::context_t *ctx;

    one_per_thread_t<namespace_cache_t> namespace_caches;

    DISABLE_COPYING(namespace_repo_t);
};

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_ */
