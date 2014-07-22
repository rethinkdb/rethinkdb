// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_
#define CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_

#include <map>

#include "clustering/administration/metadata.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "concurrency/promise.hpp"
#include "containers/clone_ptr.hpp"
#include "containers/incremental_lenses.hpp"
#include "rdb_protocol/context.hpp"

/* `namespace_repo_t` is responsible for providing `namespace_interface_t` objects to the
ReQL code, by subclassing `base_namespace_interface_t`. Internally, it constructs and
caches `cluster_namespace_interface_t` objects. Caching `cluster_namespace_interface_t`
objects is important because every time a new `cluster_namespace_interface_t` is created,
it must perform a handshake with every `master_t`, which means several network
round-trips. */

class mailbox_manager_t;
class namespace_interface_t;
class namespaces_directory_metadata_t;
class peer_id_t;
class rdb_context_t;
class signal_t;
class uuid_u;
template <class> class watchable_t;

class namespace_repo_t : public base_namespace_repo_t, public home_thread_mixin_t {
private:
    struct namespace_cache_t;

public:
    namespace_repo_t(mailbox_manager_t *,
                     const boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > &semilattice_view,
                     clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> > >,
                     rdb_context_t *);
    ~namespace_repo_t();

    bool check_namespace_exists(const uuid_u &ns_id, signal_t *interruptor);

private:
    void create_and_destroy_namespace_interface(
            namespace_cache_t *cache,
            const uuid_u &namespace_id,
            auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING;
    void on_namespaces_change(auto_drainer_t::lock_t keepalive);

    base_namespace_repo_t::namespace_cache_entry_t *get_cache_entry(const uuid_u &ns_id);

    mailbox_manager_t *mailbox_manager;
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > namespaces_view;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> > > namespaces_directory_metadata;
    rdb_context_t *ctx;

    one_per_thread_t<std::map<namespace_id_t, std::map<key_range_t, machine_id_t> > >
        region_to_primary_maps;
    one_per_thread_t<namespace_cache_t> namespace_caches;

    DISABLE_COPYING(namespace_repo_t);

    auto_drainer_t drainer;

    // We must destroy the subscription before the drainer
    semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> >::subscription_t namespaces_subscription;
};

#endif /* CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP_ */
