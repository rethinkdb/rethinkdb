#ifndef __MEMCACHED_CLUSTERING_HPP__
#define __MEMCACHED_CLUSTERING_HPP__

#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "memcached/protocol.hpp"

void serve_clustered_memcached(int port, int n_slices, cluster_namespace_interface_t<memcached_protocol_t>& namespace_if_);

#endif /* __MEMCACHED_CLUSTERING_HPP__ */

