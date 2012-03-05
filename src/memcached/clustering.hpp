#ifndef __MEMCACHED_CLUSTERING_HPP__
#define __MEMCACHED_CLUSTERING_HPP__

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "memcached/protocol.hpp"
#include "memcached/tcp_conn.hpp"
#include "rpc/semilattice/view.hpp"
#include "clustering/administration/namespace_metadata.hpp"

void serve_clustered_memcached(int port, int n_slices, cluster_namespace_interface_t<memcached_protocol_t>& namespace_if_);

class memcached_parser_maker_t {
public:
    memcached_parser_maker_t(mailbox_manager_t *, 
                             boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > >,
                             clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<memcached_protocol_t> > >);

private:
    void on_change();

    mailbox_manager_t *mailbox_manager;
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> > > namespaces_semilattice_metadata;
    clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<memcached_protocol_t> > > namespaces_directory_metadata;

    struct parser_and_namespace_if_t {
        parser_and_namespace_if_t(namespace_id_t, memcached_parser_maker_t *parent, int port);

        cluster_namespace_interface_t<memcached_protocol_t> namespace_if;
        memcache_listener_t parser;
    };

    boost::ptr_map<namespace_id_t, parser_and_namespace_if_t> parsers;

    semilattice_read_view_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::subscription_t subscription;
};

#endif /* __MEMCACHED_CLUSTERING_HPP__ */

