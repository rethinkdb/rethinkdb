#include "unittest/gtest.hpp"

#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/immediate_consistency/query/namespace_interface.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/reactor/reactor.hpp"
#include "concurrency/watchable.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/dummy_protocol_json_adapter.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

void generate_sample_region(int i, int n, dummy_protocol_t::region_t *out) {
    *out = dummy_protocol_t::region_t('a' + ((i * 26)/n), 'a' + (((i + 1) * 26)/n) - 1);
}

template<class protocol_t>
bool is_blueprint_satisfied(const blueprint_t<protocol_t> &bp,
                            const std::map<peer_id_t, boost::optional<reactor_business_card_t<protocol_t> > > &reactor_directory) {
    for (typename blueprint_t<protocol_t>::role_map_t::const_iterator it  = bp.peers_roles.begin();
                                                                      it != bp.peers_roles.end();
                                                                      it++) {

        if (reactor_directory.find(it->first) == reactor_directory.end() ||
            !reactor_directory.find(it->first)->second) {
            return false;
        }
        reactor_business_card_t<protocol_t> bcard = reactor_directory.find(it->first)->second.get();

        for (typename blueprint_t<protocol_t>::region_to_role_map_t::const_iterator jt  = it->second.begin();
                                                                                    jt != it->second.end();
                                                                                    jt++) {
            bool found = false;
            for (typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator kt  = bcard.activities.begin();
                                                                                              kt != bcard.activities.end();
                                                                                              kt++) {
                if (jt->first == kt->second.first) {
                    if (jt->second == blueprint_details::role_primary &&
                            boost::get<typename reactor_business_card_t<protocol_t>::primary_t>(&kt->second.second) &&
                            boost::get<typename reactor_business_card_t<protocol_t>::primary_t>(kt->second.second).replier.is_initialized()) {
                        found = true;
                        break;
                    } else if (jt->second == blueprint_details::role_secondary &&
                            boost::get<typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t>(&kt->second.second)) {
                        found = true;
                        break;
                    } else if (jt->second == blueprint_details::role_nothing &&
                            boost::get<typename reactor_business_card_t<protocol_t>::nothing_t>(&kt->second.second)) {
                        found = true;
                        break;
                    } else {
                        return false;
                    }
                }
            }

            if (!found) {
                return false;
            }
        }
    }
    return true;
}

template<class protocol_t>
class test_cluster_directory_t {
public:
    boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > reactor_directory;
    std::map<master_id_t, master_business_card_t<protocol_t> > master_directory;

    RDB_MAKE_ME_SERIALIZABLE_2(reactor_directory, master_directory);
};

/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
template<class protocol_t>
class reactor_test_cluster_t {
public:
    explicit reactor_test_cluster_t(int port) :
        connectivity_cluster(),
        message_multiplexer(&connectivity_cluster),

        mailbox_manager_client(&message_multiplexer, 'M'),
        mailbox_manager(&mailbox_manager_client),
        mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),

        semilattice_manager_client(&message_multiplexer, 'S'),
        semilattice_manager_branch_history(&semilattice_manager_client, branch_history_t<protocol_t>()),
        semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_branch_history),

        our_directory_variable(test_cluster_directory_t<protocol_t>()),
        directory_manager_client(&message_multiplexer, 'D'),
        directory_read_manager(&connectivity_cluster),
        directory_write_manager(&directory_manager_client, our_directory_variable.get_watchable()),
        directory_manager_client_run(&directory_manager_client, &directory_read_manager),

        message_multiplexer_run(&message_multiplexer),
        connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run)
        { }

    peer_id_t get_me() {
        return connectivity_cluster.get_me();
    }

    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;

    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;

    message_multiplexer_t::client_t semilattice_manager_client;
    semilattice_manager_t<branch_history_t<protocol_t> > semilattice_manager_branch_history;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;

    watchable_variable_t<test_cluster_directory_t<protocol_t> > our_directory_variable;
    message_multiplexer_t::client_t directory_manager_client;
    directory_read_manager_t<test_cluster_directory_t<protocol_t> > directory_read_manager;
    directory_write_manager_t<test_cluster_directory_t<protocol_t> > directory_write_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;

    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;
};

template<class protocol_t>
class test_reactor_t : private master_t<protocol_t>::ack_checker_t {
public:
    test_reactor_t(reactor_test_cluster_t<protocol_t> *r, const blueprint_t<protocol_t> &initial_blueprint, multistore_ptr_t<protocol_t> *svs) :
        blueprint_watchable(initial_blueprint),
        reactor(&r->mailbox_manager, this,
              r->directory_read_manager.get_root_view()->subview(&test_reactor_t<protocol_t>::extract_reactor_directory),
              r->semilattice_manager_branch_history.get_root_view(), blueprint_watchable.get_watchable(), svs, &get_global_perfmon_collection()),
        reactor_directory_copier(&test_cluster_directory_t<protocol_t>::reactor_directory, reactor.get_reactor_directory()->subview(&test_reactor_t<protocol_t>::wrap_in_optional), &r->our_directory_variable),
        master_directory_copier(&test_cluster_directory_t<protocol_t>::master_directory, reactor.get_master_directory(), &r->our_directory_variable)
    {
        rassert(svs->get_multistore_joined_region() == a_thru_z_region());
    }

    bool is_acceptable_ack_set(const std::set<peer_id_t> &acks) {
        return acks.size() >= 1;
    }

    watchable_variable_t<blueprint_t<protocol_t> > blueprint_watchable;
    reactor_t<protocol_t> reactor;
    field_copier_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >, test_cluster_directory_t<protocol_t> > reactor_directory_copier;
    field_copier_t<std::map<master_id_t, master_business_card_t<protocol_t> >, test_cluster_directory_t<protocol_t> > master_directory_copier;

private:
    static boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > wrap_in_optional(
            const directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > &input) {
        return boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >(input);
    }

    static std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > extract_reactor_directory(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &bcards) {
        std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > out;
        for (typename std::map<peer_id_t, test_cluster_directory_t<protocol_t> >::const_iterator it = bcards.begin(); it != bcards.end(); it++) {
            out.insert(std::make_pair(it->first, it->second.reactor_directory));
        }
        return out;
    }
};

template<class protocol_t>
class test_cluster_group_t {
public:
    boost::ptr_vector<temp_file_t> files;
    boost::ptr_vector<typename protocol_t::store_t> stores;
    boost::ptr_vector<multistore_ptr_t<protocol_t> > svses;
    boost::ptr_vector<reactor_test_cluster_t<protocol_t> > test_clusters;

    boost::ptr_vector<test_reactor_t<protocol_t> > test_reactors;

    std::map<std::string, std::string> inserter_state;

    explicit test_cluster_group_t(int n_machines) {
        int port = randport();
        for (int i = 0; i < n_machines; i++) {
            files.push_back(new temp_file_t("/tmp/rdb_unittest.XXXXXX"));
            stores.push_back(new typename protocol_t::store_t(files[i].name(), true, NULL));
            store_view_t<protocol_t> *store_ptr = &stores[i];
            svses.push_back(new multistore_ptr_t<protocol_t>(&store_ptr, 1));
            stores.back().metainfo.set(a_thru_z_region(), binary_blob_t(version_range_t(version_t::zero())));

            test_clusters.push_back(new reactor_test_cluster_t<protocol_t>(port + i));
            if (i > 0) {
                test_clusters[0].connectivity_cluster_run.join(test_clusters[i].connectivity_cluster.get_peer_address(test_clusters[i].connectivity_cluster.get_me()));
            }
        }
    }

    void construct_all_reactors(const blueprint_t<protocol_t> &bp) {
        for (unsigned i = 0; i < test_clusters.size(); i++) {
            test_reactors.push_back(new test_reactor_t<protocol_t>(&test_clusters[i], bp, &svses[i]));
        }
    }

    peer_id_t get_peer_id(unsigned i) {
        rassert(i < test_clusters.size());
        return test_clusters[i].get_me();
    }

    blueprint_t<protocol_t> compile_blueprint(const std::string& bp) {
        blueprint_t<protocol_t> blueprint;

        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        typedef tokenizer::iterator tok_iterator;

        boost::char_separator<char> sep(",");
        tokenizer tokens(bp, sep);

        unsigned peer = 0;
        for (tok_iterator it =  tokens.begin();
                          it != tokens.end();
                          it++) {

            blueprint.add_peer(get_peer_id(peer));
            for (unsigned i = 0; i < it->size(); i++) {
                typename protocol_t::region_t region;
                generate_sample_region(i, it->size(), &region);

                switch (it->at(i)) {
                    case 'p':
                        blueprint.add_role(get_peer_id(peer), region, blueprint_details::role_primary);
                        break;
                    case 's':
                        blueprint.add_role(get_peer_id(peer), region, blueprint_details::role_secondary);
                        break;
                    case 'n':
                        blueprint.add_role(get_peer_id(peer), region, blueprint_details::role_nothing);
                        break;
                    default:
                        crash("Bad blueprint string\n");
                        break;
                }
            }
            peer++;
        }
        return blueprint;
    }

    void set_all_blueprints(const blueprint_t<protocol_t> &bp) {
        for (unsigned i = 0; i < test_clusters.size(); i++) {
            test_reactors[i].blueprint_watchable.set_value(bp);
        }
    }

    //void set_blueprint(unsigned i, const blueprint_t<protocol_t> &bp) {
    //    test_reactors[i].blueprint_watchable.set_value(bp);
    //}

    static std::map<peer_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > > extract_master_directory(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input) {
        std::map<peer_id_t, std::map<master_id_t, master_business_card_t<protocol_t> > > output;
        for (typename std::map<peer_id_t, test_cluster_directory_t<protocol_t> >::const_iterator it = input.begin(); it != input.end(); it++) {
            output.insert(std::make_pair(it->first, it->second.master_directory));
        }
        return output;
    }

    void run_queries() {
        nap(200);
        for (unsigned i = 0; i < test_clusters.size(); i++) {
            cluster_namespace_interface_t<protocol_t> namespace_if(&test_clusters[i].mailbox_manager,
                (&test_clusters[i])->directory_read_manager.get_root_view()
                    ->subview(&test_cluster_group_t::extract_master_directory));
            namespace_if.get_initial_ready_signal()->wait_lazily_unordered();

            order_source_t order_source;

            test_inserter_t inserter(&namespace_if, &key_gen<protocol_t>, &order_source, "test_cluster_group_t::run_queries/inserter", &inserter_state);
            let_stuff_happen();
            inserter.stop();
            inserter.validate();
        }
    }

    static std::map<peer_id_t, boost::optional<reactor_business_card_t<protocol_t> > > extract_reactor_business_cards(
            const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input) {
        std::map<peer_id_t, boost::optional<reactor_business_card_t<protocol_t> > > out;
        for (typename std::map<peer_id_t, test_cluster_directory_t<protocol_t> >::const_iterator it = input.begin(); it != input.end(); it++) {
            if (it->second.reactor_directory) {
                out.insert(std::make_pair(it->first, boost::optional<reactor_business_card_t<protocol_t> >(it->second.reactor_directory->internal)));
            } else {
                out.insert(std::make_pair(it->first, boost::optional<reactor_business_card_t<protocol_t> >()));
            }
        }
        return out;
    }

    void wait_until_blueprint_is_satisfied(const blueprint_t<protocol_t> &bp) {
        try {
            const int timeout_ms = 60000;
            signal_timer_t timer(timeout_ms);
            test_clusters[0].directory_read_manager.get_root_view()
                ->subview(&test_cluster_group_t<protocol_t>::extract_reactor_business_cards)
                    ->run_until_satisfied(boost::bind(&is_blueprint_satisfied<protocol_t>, bp, _1), &timer);
        } catch (interrupted_exc_t) {
            ADD_FAILURE() << "The blueprint took too long to be satisfied, this is probably an error but you could try increasing the timeout.";
        }

        nap(100);
    }

    void wait_until_blueprint_is_satisfied(const std::string& bp) {
        wait_until_blueprint_is_satisfied(compile_blueprint(bp));
    }
};

}   /* anonymous namespace */

void runOneShardOnePrimaryOneNodeStartupShutdowntest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);
    nap(100);
    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OneShardOnePrimaryOneNodeStartupShutdown) {
    run_in_thread_pool(&runOneShardOnePrimaryOneNodeStartupShutdowntest);
}

void runOneShardOnePrimaryOneSecondaryStartupShutdowntest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(3);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));

    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OneShardOnePrimaryOneSecondaryStartupShutdowntest) {
    run_in_thread_pool(&runOneShardOnePrimaryOneSecondaryStartupShutdowntest);
}

void runTwoShardsTwoNodes() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("ps,sp"));

    cluster_group.wait_until_blueprint_is_satisfied("ps,sp");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, TwoShardsTwoNodes) {
    run_in_thread_pool(&runTwoShardsTwoNodes);
}

void runRoleSwitchingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");

    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("n,p"));
    cluster_group.wait_until_blueprint_is_satisfied("n,p");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, RoleSwitchingTest) {
    run_in_thread_pool(&runRoleSwitchingTest);
}

void runOtherRoleSwitchingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("s,p"));
    cluster_group.wait_until_blueprint_is_satisfied("s,p");

    cluster_group.run_queries();
}

TEST(ClusteringReactor, OtherRoleSwitchingTest) {
    run_in_thread_pool(&runOtherRoleSwitchingTest);
}

void runAddSecondaryTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(3);
    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,s,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("p,s,s"));
    cluster_group.wait_until_blueprint_is_satisfied("p,s,s");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, AddSecondaryTest) {
    run_in_thread_pool(&runAddSecondaryTest);
}

void runReshardingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pp,ns"));
    cluster_group.wait_until_blueprint_is_satisfied("pp,ns");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pn,np"));
    cluster_group.wait_until_blueprint_is_satisfied("pn,np");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, ReshardingTest) {
    run_in_thread_pool(&runReshardingTest);
}

void runLessGracefulReshardingTest() {
    test_cluster_group_t<dummy_protocol_t> cluster_group(2);

    cluster_group.construct_all_reactors(cluster_group.compile_blueprint("p,n"));
    cluster_group.wait_until_blueprint_is_satisfied("p,n");
    cluster_group.run_queries();

    cluster_group.set_all_blueprints(cluster_group.compile_blueprint("pn,np"));
    cluster_group.wait_until_blueprint_is_satisfied("pn,np");
    cluster_group.run_queries();
}

TEST(ClusteringReactor, LessGracefulReshardingTest) {
    run_in_thread_pool(&runLessGracefulReshardingTest);
}

} // namespace unittest
