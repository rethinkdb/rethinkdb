#include "unittest/clustering_utils.hpp"

namespace unittest {

peer_address_t get_cluster_local_address(connectivity_cluster_t *cm) {
    auto_drainer_t::lock_t connection_keepalive;
    connectivity_cluster_t::connection_t *loopback =
        cm->get_connection(cm->get_me(), &connection_keepalive);
    guarantee(loopback != NULL);
    return loopback->get_peer_address();
}

} // namespace unittest

