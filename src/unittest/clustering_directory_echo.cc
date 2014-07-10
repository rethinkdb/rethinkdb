// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/reactor/directory_echo.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"

namespace unittest {

namespace {

template<class metadata_t>
class directory_echo_cluster_t {
public:
    directory_echo_cluster_t(const metadata_t &initial, int port) :
        cluster_manager(),
        mailbox_manager(&cluster_manager, 'M'),
        echo_writer(&mailbox_manager, initial),
        directory_read_manager(&cluster_manager, 'D'),
        directory_write_manager(&cluster_manager, 'D', echo_writer.get_watchable()),
        cluster_manager_run(&cluster_manager,
                            get_unittest_addresses(),
                            peer_address_t(),
                            port, 0),
        echo_mirror(&mailbox_manager, directory_read_manager.get_root_view())
        { }
    cluster_manager_t cluster_manager;
    mailbox_manager_t mailbox_manager;
    directory_echo_writer_t<metadata_t> echo_writer;
    directory_read_manager_t<directory_echo_wrapper_t<metadata_t> > directory_read_manager;
    directory_write_manager_t<directory_echo_wrapper_t<metadata_t> > directory_write_manager;
    cluster_manager_t::run_t cluster_manager_run;
    directory_echo_mirror_t<metadata_t> echo_mirror;
};

}   /* anonymous namespace */

TPTEST(ClusteringDirectoryEcho, DirectoryEcho) {
    directory_echo_cluster_t<std::string> cluster1("hello", ANY_PORT), cluster2("world", ANY_PORT);
    cluster1.cluster_manager_run.join(get_cluster_local_address(&cluster2.cluster_manager));

    directory_echo_version_t version;
    {
        directory_echo_writer_t<std::string>::our_value_change_t change(&cluster1.echo_writer);
        change.buffer = "Hello";
        version = change.commit();
    }
    directory_echo_writer_t<std::string>::ack_waiter_t waiter(&cluster1.echo_writer, cluster2.cluster_manager.get_me(), version);
    waiter.wait_lazily_unordered();
    std::string cluster2_sees_cluster1_directory_as = cluster2.echo_mirror.get_internal()->get().get_inner().find(cluster1.cluster_manager.get_me())->second;
    EXPECT_EQ("Hello", cluster2_sees_cluster1_directory_as);
}

}   /* namespace unittest */
