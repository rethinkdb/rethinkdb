// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/reactor/directory_echo.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"

namespace unittest {

namespace {

template<class metadata_t>
class directory_echo_cluster_t {
public:
    directory_echo_cluster_t(const metadata_t &initial, int port) :
        connectivity_cluster(),
        message_multiplexer(&connectivity_cluster),
        heartbeat_manager_client(&message_multiplexer, 'H'),
        heartbeat_manager(&heartbeat_manager_client),
        heartbeat_manager_client_run(&heartbeat_manager_client, &heartbeat_manager),
        mailbox_manager_client(&message_multiplexer, 'M'),
        mailbox_manager(&mailbox_manager_client),
        mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),
        echo_writer(&mailbox_manager, initial),
        directory_manager_client(&message_multiplexer, 'D'),
        directory_read_manager(&connectivity_cluster),
        directory_write_manager(&directory_manager_client, echo_writer.get_watchable()),
        directory_manager_client_run(&directory_manager_client, &directory_read_manager),
        message_multiplexer_run(&message_multiplexer),
        connectivity_cluster_run(&connectivity_cluster,
                                 get_unittest_addresses(),
                                 peer_address_t(),
                                 port,
                                 &message_multiplexer_run,
                                 0,
                                 &heartbeat_manager),
        echo_mirror(&mailbox_manager, directory_read_manager.get_root_view())
        { }
    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;
    message_multiplexer_t::client_t heartbeat_manager_client;
    heartbeat_manager_t heartbeat_manager;
    message_multiplexer_t::client_t::run_t heartbeat_manager_client_run;
    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;
    directory_echo_writer_t<metadata_t> echo_writer;
    message_multiplexer_t::client_t directory_manager_client;
    directory_read_manager_t<directory_echo_wrapper_t<metadata_t> > directory_read_manager;
    directory_write_manager_t<directory_echo_wrapper_t<metadata_t> > directory_write_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;
    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;
    directory_echo_mirror_t<metadata_t> echo_mirror;
};

}   /* anonymous namespace */

void run_directory_echo_test() {
    directory_echo_cluster_t<std::string> cluster1("hello", ANY_PORT), cluster2("world", ANY_PORT);
    cluster1.connectivity_cluster_run.join(cluster2.connectivity_cluster.get_peer_address(cluster2.connectivity_cluster.get_me()));

    directory_echo_version_t version;
    {
        directory_echo_writer_t<std::string>::our_value_change_t change(&cluster1.echo_writer);
        change.buffer = "Hello";
        version = change.commit();
    }
    directory_echo_writer_t<std::string>::ack_waiter_t waiter(&cluster1.echo_writer, cluster2.connectivity_cluster.get_me(), version);
    waiter.wait_lazily_unordered();
    std::string cluster2_sees_cluster1_directory_as = cluster2.echo_mirror.get_internal()->get().find(cluster1.connectivity_cluster.get_me())->second;
    EXPECT_EQ("Hello", cluster2_sees_cluster1_directory_as);
}
TEST(ClusteringDirectoryEcho, DirectoryEcho) {
    unittest::run_in_thread_pool(&run_directory_echo_test);
}

}   /* namespace unittest */
