#ifndef __CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP__
#define __CLUSTERING_ADMINISTRATION_NAMESPACE_INTERFACE_REPOSITORY_HPP__

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "clustering/administration/namespace_metadata.hpp"

/* `namespace_repo_t` is responsible for constructing and caching
`cluster_namespace_interface_t` objects for all of the namespaces in the cluster
for a given protocol. Caching `cluster_namespace_interface_t` objects is
important because every time a new `cluster_namespace_interface_t` is created,
it must perform a handshake with every `master_t`, which means several network
round-trips. */

template <class protocol_t>
class namespace_repo_t {
public:
    namespace_repo_t(mailbox_manager_t *, 
                     clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > >);

    struct access_t {
        access_t(namespace_repo_t *repo, namespace_id_t ns_id);
        cluster_namespace_interface_t<protocol_t> *get_namespace_if() {
            return parent->interface_map.find(ns_id)->second;
        }
    private:
        namespace_repo_t *parent;
        namespace_id_t ns_id;
        /* We cut corners by not bothering to clean up namespace interfaces that
        are dead. If we did clean up dead namespace interfaces, we would have to
        have a lock here so that they didn't get cleaned up while in use. */
    };

private:
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > namespaces_directory_metadata;

    boost::ptr_map<namespace_id_t, cluster_namespace_interface_t<protocol_t> > interface_map;
};

#include "clustering/administration/namespace_interface_repository.tcc"

#endif
