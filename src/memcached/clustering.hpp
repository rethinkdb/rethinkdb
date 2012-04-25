#ifndef MEMCACHED_CLUSTERING_HPP_
#define MEMCACHED_CLUSTERING_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "memcached/tcp_conn.hpp"
#include "rpc/semilattice/view.hpp"

void serve_clustered_memcached(int port, int n_slices, cluster_namespace_interface_t<memcached_protocol_t>& namespace_if_);

class memcached_parser_maker_t {
public:
    memcached_parser_maker_t(mailbox_manager_t *,
                             boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > >,
#ifndef NDEBUG
                             boost::shared_ptr<semilattice_read_view_t<machine_semilattice_metadata_t> >,
#endif
                             namespace_repo_t<memcached_protocol_t> *_namespace_repo);

private:
    void on_change();

    mailbox_manager_t *mailbox_manager;
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > > namespaces_semilattice_metadata;
#ifndef NDEBUG
    boost::shared_ptr<semilattice_read_view_t<machine_semilattice_metadata_t> > machine_semilattice_metadata;
#endif

    struct parser_and_namespace_if_t {
        parser_and_namespace_if_t(namespace_id_t, memcached_parser_maker_t *parent, int port);

        namespace_repo_t<memcached_protocol_t>::namepsace_if_access_t namespace_if_access;
        memcache_listener_t parser;
    };

public:
    typedef boost::ptr_map<namespace_id_t, parser_and_namespace_if_t> parser_map_t;
private:
    parser_map_t parsers;

#ifndef NDEBUG
    semilattice_read_view_t<machine_semilattice_metadata_t>::subscription_t machines_subscription;
#endif
    semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::subscription_t namespaces_subscription;
    namespace_repo_t<memcached_protocol_t> *namespace_repo;
};

#endif /* MEMCACHED_CLUSTERING_HPP_ */

