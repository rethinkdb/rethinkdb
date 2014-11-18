// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_TEST_CLUSTER_GROUP_HPP_
#define UNITTEST_TEST_CLUSTER_GROUP_HPP_

#include <map>
#include <string>
#include <set>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/cow_ptr.hpp"
#include "containers/scoped.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/mock_store.hpp"

class blueprint_t;
class cluster_namespace_interface_t;
class io_backender_t;
class multistore_ptr_t;
class reactor_business_card_t;
class peer_id_t;
class serializer_t;

namespace unittest {

class temp_file_t;

class reactor_test_cluster_t;
class test_reactor_t;

class test_cluster_group_t {
public:
    const base_path_t base_path;
    std::vector<scoped_ptr_t<temp_file_t> > files;
    scoped_ptr_t<io_backender_t> io_backender;
    scoped_ptr_t<cache_balancer_t> balancer;
    std::vector<scoped_ptr_t<serializer_t> > serializers;
    std::vector<scoped_ptr_t<mock_store_t> > stores;
    std::vector<scoped_ptr_t<multistore_ptr_t> > svses;
    std::vector<scoped_ptr_t<reactor_test_cluster_t> > test_clusters;

    std::vector<scoped_ptr_t<test_reactor_t> > test_reactors;

    std::map<std::string, std::string> inserter_state;

    rdb_context_t ctx;

    explicit test_cluster_group_t(int n_servers);
    ~test_cluster_group_t();

    void construct_all_reactors(const blueprint_t &bp);

    peer_id_t get_peer_id(size_t i);

    blueprint_t compile_blueprint(const std::string& bp);

    void set_all_blueprints(const blueprint_t &bp);

    scoped_ptr_t<cluster_namespace_interface_t> make_namespace_interface(int i);

    void run_queries();

    void wait_until_blueprint_is_satisfied(const blueprint_t &bp);

    void wait_until_blueprint_is_satisfied(const std::string& bp);
};

/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
class reactor_test_cluster_t {
public:
    explicit reactor_test_cluster_t(int port);
    ~reactor_test_cluster_t();

    peer_id_t get_me();

    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager;
    directory_read_manager_t<namespace_directory_metadata_t> directory_read_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    in_memory_branch_history_manager_t branch_history_manager;
};

}  // namespace unittest

#endif  // UNITTEST_TEST_CLUSTER_GROUP_HPP_
