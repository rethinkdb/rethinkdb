#include "unittest/simple_mailbox_cluster.hpp"

#include "clustering/administration/metadata.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"


namespace unittest {

struct simple_mailbox_cluster_t::simple_mailbox_cluster_state {
    connectivity_cluster_t connectivity_cluster;
    server_id_t server_id;
    mailbox_manager_t mailbox_manager;
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t> heartbeat_manager;
    auth_semilattice_metadata_t auth_semilattice_metadata;
    dummy_semilattice_controller_t<auth_semilattice_metadata_t> auth_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    simple_mailbox_cluster_state() :
        server_id(server_id_t::generate_server_id()),
        mailbox_manager(&connectivity_cluster, 'M'),
        heartbeat_manager(heartbeat_semilattice_metadata),
        auth_manager(auth_semilattice_metadata),
        connectivity_cluster_run(&connectivity_cluster,
                                 server_id,
                                 get_unittest_addresses(),
                                 peer_address_t(),
                                 0,
                                 ANY_PORT,
                                 0,
                                 heartbeat_manager.get_view(),
                                 auth_manager.get_view(),
                                 nullptr) {}
};

simple_mailbox_cluster_t::simple_mailbox_cluster_t()
    : state(make_scoped<simple_mailbox_cluster_state>()) {}

simple_mailbox_cluster_t::~simple_mailbox_cluster_t() {}

connectivity_cluster_t *simple_mailbox_cluster_t::get_connectivity_cluster() {
    return &state->connectivity_cluster;
}
mailbox_manager_t *simple_mailbox_cluster_t::get_mailbox_manager() {
    return &state->mailbox_manager;
}

void simple_mailbox_cluster_t::connect(simple_mailbox_cluster_t *other) {
    state->connectivity_cluster_run.join(
        get_cluster_local_address(&other->state->connectivity_cluster), 0);
}

void simple_mailbox_cluster_t::disconnect(simple_mailbox_cluster_t *other) {
    auto_drainer_t::lock_t keepalive;
    connectivity_cluster_t::connection_t *conn = state->connectivity_cluster.get_connection(
        other->state->connectivity_cluster.get_me(),
        &keepalive);
    guarantee(conn != nullptr);
    conn->kill_connection();
}

}  // namespace unittest

