// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_TEST_CLUSTER_GROUP_HPP_
#define UNITTEST_TEST_CLUSTER_GROUP_HPP_

#include <map>
#include <string>
#include <set>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "containers/cow_ptr.hpp"
#include "containers/scoped.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "unittest/branch_history_manager.hpp"

template <class> class blueprint_t;
template <class> class cluster_namespace_interface_t;
class io_backender_t;
template <class> class multistore_ptr_t;
template <class> class reactor_business_card_t;
class peer_id_t;
class serializer_t;

namespace unittest {

class temp_file_t;

template <class> class reactor_test_cluster_t;
template <class> class test_reactor_t;

template<class protocol_t>
class test_cluster_directory_t {
public:
    directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > reactor_directory;

    RDB_MAKE_ME_SERIALIZABLE_1(reactor_directory);
};


template<class protocol_t>
class test_cluster_group_t {
public:
    const base_path_t base_path;
    boost::ptr_vector<temp_file_t> files;
    scoped_ptr_t<io_backender_t> io_backender;
    boost::ptr_vector<serializer_t> serializers;
    boost::ptr_vector<typename protocol_t::store_t> stores;
    boost::ptr_vector<multistore_ptr_t<protocol_t> > svses;
    boost::ptr_vector<reactor_test_cluster_t<protocol_t> > test_clusters;

    boost::ptr_vector<test_reactor_t<protocol_t> > test_reactors;

    std::map<std::string, std::string> inserter_state;

    typename protocol_t::context_t ctx;

    explicit test_cluster_group_t(int n_machines);
    ~test_cluster_group_t();

    void construct_all_reactors(const blueprint_t<protocol_t> &bp);

    peer_id_t get_peer_id(unsigned i);

    blueprint_t<protocol_t> compile_blueprint(const std::string& bp);

    void set_all_blueprints(const blueprint_t<protocol_t> &bp);

    static std::map<peer_id_t, cow_ptr_t<reactor_business_card_t<protocol_t> > > extract_reactor_business_cards_from_test_cluster_directory(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input);

    void make_namespace_interface(int i, scoped_ptr_t<cluster_namespace_interface_t<protocol_t> > *out);

    void run_queries();

    static std::map<peer_id_t, boost::optional<cow_ptr_t<reactor_business_card_t<protocol_t> > > > extract_reactor_business_cards(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input);

    void wait_until_blueprint_is_satisfied(const blueprint_t<protocol_t> &bp);

    void wait_until_blueprint_is_satisfied(const std::string& bp);
};

/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
template<class protocol_t>
class reactor_test_cluster_t {
public:
    explicit reactor_test_cluster_t(int port);
    ~reactor_test_cluster_t();

    peer_id_t get_me();

    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;

    message_multiplexer_t::client_t heartbeat_manager_client;
    heartbeat_manager_t heartbeat_manager;
    message_multiplexer_t::client_t::run_t heartbeat_manager_client_run;

    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;

    watchable_variable_t<test_cluster_directory_t<protocol_t> > our_directory_variable;
    message_multiplexer_t::client_t directory_manager_client;
    directory_read_manager_t<test_cluster_directory_t<protocol_t> > directory_read_manager;
    directory_write_manager_t<test_cluster_directory_t<protocol_t> > directory_write_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;

    message_multiplexer_t::run_t message_multiplexer_run;

    connectivity_cluster_t::run_t connectivity_cluster_run;

    in_memory_branch_history_manager_t<protocol_t> branch_history_manager;
};

}  // namespace unittest

#endif  // UNITTEST_TEST_CLUSTER_GROUP_HPP_
