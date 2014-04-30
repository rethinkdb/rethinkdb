// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/test_cluster_group.hpp"

#include <map>
#include <string>
#include <set>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

#include "arch/io/io_utils.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/immediate_consistency/branch/backfill_throttler.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "clustering/reactor/reactor.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "concurrency/watchable.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rdb_protocol/protocol.hpp"

#include "rpc/directory/read_manager.tcc"
#include "rpc/directory/write_manager.tcc"


namespace unittest {

RDB_IMPL_ME_SERIALIZABLE_1(test_cluster_directory_t, 0, reactor_directory);


void generate_sample_region(int i, int n, region_t *out) {
    // We keep old dummy-protocol style single-character key logic.
    *out = hash_region_t<key_range_t>(key_range_t(
                                              key_range_t::closed,
                                              store_key_t(std::string(1, 'a' + ((i * 26)/n))),
                                              key_range_t::open,
                                              store_key_t(std::string(1, 'a' + (((i + 1) * 26)/n)))));
}

bool is_blueprint_satisfied(const blueprint_t &bp,
                            const std::map<peer_id_t, boost::optional<cow_ptr_t<reactor_business_card_t> > > &reactor_directory) {
    for (blueprint_t::role_map_t::const_iterator it  = bp.peers_roles.begin();
         it != bp.peers_roles.end();
         ++it) {

        if (reactor_directory.find(it->first) == reactor_directory.end() ||
            !reactor_directory.find(it->first)->second) {
            return false;
        }
        reactor_business_card_t bcard = *reactor_directory.find(it->first)->second.get();

        for (blueprint_t::region_to_role_map_t::const_iterator jt = it->second.begin();
             jt != it->second.end();
             ++jt) {
            bool found = false;
            for (reactor_business_card_t::activity_map_t::const_iterator kt = bcard.activities.begin();
                 kt != bcard.activities.end();
                 ++kt) {
                if (jt->first == kt->second.region) {
                    if (jt->second == blueprint_role_primary &&
                        boost::get<reactor_business_card_t::primary_t>(&kt->second.activity) &&
                        boost::get<reactor_business_card_t::primary_t>(kt->second.activity).replier.is_initialized()) {
                        found = true;
                        break;
                    } else if (jt->second == blueprint_role_secondary &&
                               boost::get<reactor_business_card_t::secondary_up_to_date_t>(&kt->second.activity)) {
                        found = true;
                        break;
                    } else if (jt->second == blueprint_role_nothing &&
                               boost::get<reactor_business_card_t::nothing_t>(&kt->second.activity)) {
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

class test_reactor_t : private ack_checker_t {
public:
    test_reactor_t(const base_path_t &base_path, io_backender_t *io_backender, reactor_test_cluster_t *r, const blueprint_t &initial_blueprint, multistore_ptr_t *svs);
    ~test_reactor_t();
    bool is_acceptable_ack_set(const std::set<peer_id_t> &acks);
    write_durability_t get_write_durability(const peer_id_t &) const {
        return write_durability_t::SOFT;
    }

    watchable_variable_t<blueprint_t> blueprint_watchable;
    backfill_throttler_t backfill_throttler;
    reactor_t reactor;
    field_copier_t<boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >, test_cluster_directory_t> reactor_directory_copier;

private:
    static boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > wrap_in_optional(const directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > &input);

    static change_tracking_map_t<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > > extract_reactor_directory(const change_tracking_map_t<peer_id_t, test_cluster_directory_t> &bcards);
};



/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
reactor_test_cluster_t::reactor_test_cluster_t(int port) :
    connectivity_cluster(),
    message_multiplexer(&connectivity_cluster),

    heartbeat_manager_client(&message_multiplexer, 'H'),
    heartbeat_manager(&heartbeat_manager_client),
    heartbeat_manager_client_run(&heartbeat_manager_client, &heartbeat_manager),

    mailbox_manager_client(&message_multiplexer, 'M'),
    mailbox_manager(&mailbox_manager_client),
    mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),

    our_directory_variable(test_cluster_directory_t()),
    directory_manager_client(&message_multiplexer, 'D'),
    directory_read_manager(&connectivity_cluster),
    directory_write_manager(&directory_manager_client, our_directory_variable.get_watchable()),
    directory_manager_client_run(&directory_manager_client, &directory_read_manager),

    message_multiplexer_run(&message_multiplexer),
    connectivity_cluster_run(&connectivity_cluster,
                             get_unittest_addresses(),
                             peer_address_t(),
                             port,
                             &message_multiplexer_run,
                             0,
                             &heartbeat_manager) { }

reactor_test_cluster_t::~reactor_test_cluster_t() { }

peer_id_t reactor_test_cluster_t::get_me() {
    return connectivity_cluster.get_me();
}

test_reactor_t::test_reactor_t(const base_path_t &base_path, io_backender_t *io_backender, reactor_test_cluster_t *r, const blueprint_t &initial_blueprint, multistore_ptr_t *svs) :
    blueprint_watchable(initial_blueprint),
    reactor(base_path, io_backender, &r->mailbox_manager, &backfill_throttler, this,
            r->directory_read_manager.get_root_view()->subview(&test_reactor_t::extract_reactor_directory),
            &r->branch_history_manager, blueprint_watchable.get_watchable(), svs, &get_global_perfmon_collection(), NULL),
    reactor_directory_copier(&test_cluster_directory_t::reactor_directory, reactor.get_reactor_directory()->subview(&test_reactor_t::wrap_in_optional), &r->our_directory_variable) {
    rassert(svs->get_region() == region_t::universe());
}

test_reactor_t::~test_reactor_t() { }

bool test_reactor_t::is_acceptable_ack_set(const std::set<peer_id_t> &acks) {
    return acks.size() >= 1;
}

boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >
test_reactor_t::wrap_in_optional(const directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > &input) {
    return boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >(input);
}

change_tracking_map_t<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > >
test_reactor_t::extract_reactor_directory(const change_tracking_map_t<peer_id_t, test_cluster_directory_t> &bcards) {
    change_tracking_map_t<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > > out;
    out.begin_version();
    for (auto it = bcards.get_inner().begin(); it != bcards.get_inner().end(); it++) {
        out.set_value(it->first, it->second.reactor_directory);
    }
    return out;
}


test_cluster_group_t::test_cluster_group_t(int n_machines)
    : base_path("/tmp"), io_backender(new io_backender_t(file_direct_io_mode_t::buffered_desired)),
      balancer(new dummy_cache_balancer_t(GIGABYTE)) {
    for (int i = 0; i < n_machines; i++) {
        files.push_back(new temp_file_t);
        filepath_file_opener_t file_opener(files[i].name(), io_backender.get());
        standard_serializer_t::create(&file_opener,
                                      standard_serializer_t::static_config_t());
        serializers.push_back(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                        &file_opener,
                                                        &get_global_perfmon_collection()));
        stores.push_back(new mock_store_t(binary_blob_t(version_range_t(version_t::zero()))));
        store_view_t *store_ptr = &stores[i];
        svses.push_back(new multistore_ptr_t(&store_ptr, 1));

        test_clusters.push_back(new reactor_test_cluster_t(ANY_PORT));
        if (i > 0) {
            test_clusters[0].connectivity_cluster_run.join(test_clusters[i].connectivity_cluster.get_peer_address(test_clusters[i].connectivity_cluster.get_me()));
        }
    }
}

test_cluster_group_t::~test_cluster_group_t() { }

void test_cluster_group_t::construct_all_reactors(const blueprint_t &bp) {
    for (unsigned i = 0; i < test_clusters.size(); i++) {
        test_reactors.push_back(new test_reactor_t(base_path, io_backender.get(), &test_clusters[i], bp, &svses[i]));
    }
}

peer_id_t test_cluster_group_t::get_peer_id(unsigned i) {
    rassert(i < test_clusters.size());
    return test_clusters[i].get_me();
}

blueprint_t test_cluster_group_t::compile_blueprint(const std::string &bp) {
    blueprint_t blueprint;

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
            region_t region;
            generate_sample_region(i, it->size(), &region);

            switch (it->at(i)) {
            case 'p':
                blueprint.add_role(get_peer_id(peer), region, blueprint_role_primary);
                break;
            case 's':
                blueprint.add_role(get_peer_id(peer), region, blueprint_role_secondary);
                break;
            case 'n':
                blueprint.add_role(get_peer_id(peer), region, blueprint_role_nothing);
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

void test_cluster_group_t::set_all_blueprints(const blueprint_t &bp) {
    for (unsigned i = 0; i < test_clusters.size(); i++) {
        test_reactors[i].blueprint_watchable.set_value(bp);
    }
}

std::map<peer_id_t, cow_ptr_t<reactor_business_card_t> > test_cluster_group_t::extract_reactor_business_cards_no_optional(
        const change_tracking_map_t<peer_id_t, test_cluster_directory_t> &input) {
    std::map<peer_id_t, cow_ptr_t<reactor_business_card_t> > out;
    for (auto it = input.get_inner().begin(); it != input.get_inner().end(); it++) {
        if (it->second.reactor_directory) {
            out.insert(std::make_pair(it->first, it->second.reactor_directory->internal));
        } else {
            out.insert(std::make_pair(it->first, cow_ptr_t<reactor_business_card_t>()));
        }
    }
    return out;
}

void test_cluster_group_t::make_namespace_interface(int i, scoped_ptr_t<cluster_namespace_interface_t> *out) {
    out->init(new cluster_namespace_interface_t(
                      &test_clusters[i].mailbox_manager,
                      (&test_clusters[i])->directory_read_manager.get_root_view()
                      ->subview(&test_cluster_group_t::extract_reactor_business_cards_no_optional),
                      NULL));
    (*out)->get_initial_ready_signal()->wait_lazily_unordered();
}

void test_cluster_group_t::run_queries() {
    nap(200);
    for (unsigned i = 0; i < test_clusters.size(); i++) {
        scoped_ptr_t<cluster_namespace_interface_t> namespace_if;
        make_namespace_interface(i, &namespace_if);

        order_source_t order_source;

        test_inserter_t inserter(namespace_if.get(), &dummy_key_gen, &order_source, "test_cluster_group_t::run_queries/inserter", &inserter_state);
        let_stuff_happen();
        inserter.stop();
        inserter.validate();
    }
}

std::map<peer_id_t, boost::optional<cow_ptr_t<reactor_business_card_t> > > test_cluster_group_t::extract_reactor_business_cards(
        const change_tracking_map_t<peer_id_t, test_cluster_directory_t> &input) {
    std::map<peer_id_t, boost::optional<cow_ptr_t<reactor_business_card_t> > > out;
    for (auto it = input.get_inner().begin(); it != input.get_inner().end(); it++) {
        if (it->second.reactor_directory) {
            out.insert(std::make_pair(it->first, boost::optional<cow_ptr_t<reactor_business_card_t> >(it->second.reactor_directory->internal)));
        } else {
            out.insert(std::make_pair(it->first, boost::optional<cow_ptr_t<reactor_business_card_t> >()));
        }
    }
    return out;
}

void test_cluster_group_t::wait_until_blueprint_is_satisfied(const blueprint_t &bp) {
    try {
        const int timeout_ms = 60000;
        signal_timer_t timer;
        timer.start(timeout_ms);
        test_clusters[0].directory_read_manager.get_root_view()
            ->subview(&test_cluster_group_t::extract_reactor_business_cards)
            ->run_until_satisfied(boost::bind(&is_blueprint_satisfied, bp, _1), &timer);
    } catch (const interrupted_exc_t &) {
        crash("The blueprint took too long to be satisfied, this is probably an error but you could try increasing the timeout.");
    }

    nap(100);
}

void test_cluster_group_t::wait_until_blueprint_is_satisfied(const std::string& bp) {
    wait_until_blueprint_is_satisfied(compile_blueprint(bp));
}

}  // namespace unittest

