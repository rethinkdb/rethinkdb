#include "clustering/clustering.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/directory/manager.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "clustering/architect/metadata.hpp"
#include "mock/dummy_protocol.hpp"
#include "clustering/architect/reactor_driver.hpp"

struct server_starter_t : public thread_message_t {
    boost::function<void()> fun;
    thread_pool_t *thread_pool;
    void on_thread_switch() {
        coro_t::spawn_sometime(boost::bind(&server_starter_t::run, this));
    }
    void run() {
        fun();
        thread_pool->shutdown();
    }
};

void clustering_main(int port, int contact_port);

int run_server(int argc, char *argv[]) {

    guarantee(argc == 2 || argc == 3, "rethinkdb-clustering expects 1 argument");
    int port = atoi(argv[1]);

    int contact_port = -1;
    if (argc == 3) {
        contact_port = atoi(argv[2]);
    }

    server_starter_t ss;
    thread_pool_t tp(8 /* TODO: Magic constant here. */, false /* TODO: Add --set-affinity option. */);
    ss.thread_pool = &tp;
    ss.fun = boost::bind(&clustering_main, port, contact_port);
    tp.run(&ss);

    return 0;
}

void clustering_main(int port, int contact_port) {
    os_signal_cond_t c;

    connectivity_cluster_t connectivity_cluster; 
    message_multiplexer_t message_multiplexer(&connectivity_cluster);

    message_multiplexer_t::client_t mailbox_manager_client(&message_multiplexer, 'M');
    mailbox_manager_t mailbox_manager(&mailbox_manager_client);
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager);

    message_multiplexer_t::client_t semilattice_manager_client(&message_multiplexer, 'S');
    semilattice_manager_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> > semilattice_manager_namespaces(&semilattice_manager_client, namespaces_semilattice_metadata_t<mock::dummy_protocol_t>());
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_namespaces);

    message_multiplexer_t::client_t directory_manager_client(&message_multiplexer, 'D');
    directory_readwrite_manager_t<namespaces_directory_metadata_t<mock::dummy_protocol_t> > directory_manager(&directory_manager_client, namespaces_directory_metadata_t<mock::dummy_protocol_t>());
    message_multiplexer_t::client_t::run_t directory_manager_client_run(&directory_manager_client, &directory_manager);

    message_multiplexer_t::run_t message_multiplexer_run(&message_multiplexer);
    connectivity_cluster_t::run_t connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run);

    if (contact_port != -1) {
        connectivity_cluster_run.join(peer_address_t(ip_address_t::us(), contact_port));
    }

    reactor_driver_t<mock::dummy_protocol_t> reactor_driver(&mailbox_manager,
                                                            directory_manager.get_root_view(), 
                                                            semilattice_manager_namespaces.get_root_view());
    wait_for_sigint();
}
