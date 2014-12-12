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
#include "buffer_cache/cache_balancer.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/cow_ptr_type.hpp"
#include "containers/uuid.hpp"
#include "concurrency/watchable.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rdb_protocol/protocol.hpp"

#include "rpc/directory/read_manager.tcc"
#include "rpc/directory/write_manager.tcc"


namespace unittest {

void generate_sample_region(int i, int n, region_t *out) {
    // We keep old dummy-protocol style single-character key logic.
    *out = hash_region_t<key_range_t>(key_range_t(
                                              key_range_t::closed,
                                              store_key_t(std::string(1, 'a' + ((i * 26)/n))),
                                              key_range_t::open,
                                              store_key_t(std::string(1, 'a' + (((i + 1) * 26)/n)))));
}

bool is_blueprint_satisfied(
        const blueprint_t &bp,
        watchable_map_t<peer_id_t, namespace_directory_metadata_t> *directory) {
    for (auto it = bp.peers_roles.begin(); it != bp.peers_roles.end(); ++it) {
        bool ok;
        directory->read_key(it->first, [&](const namespace_directory_metadata_t *md) {
            if (md == nullptr) {
                ok = false;
                return;
            }
            const reactor_business_card_t &bcard = *md->internal;
            for (blueprint_t::region_to_role_map_t::const_iterator jt = it->second.begin();
                 jt != it->second.end();
                 ++jt) {
                bool found = false;
                for (auto kt = bcard.activities.begin();
                        kt != bcard.activities.end();
                        ++kt) {
                    if (jt->first == kt->second.region) {
                        if (jt->second == blueprint_role_primary &&
                            boost::get<reactor_business_card_t::primary_t>(
                                &kt->second.activity) &&
                            boost::get<reactor_business_card_t::primary_t>(
                                kt->second.activity).replier.is_initialized()) {
                            found = true;
                            break;
                        } else if (jt->second == blueprint_role_secondary &&
                                boost::get<reactor_business_card_t::
                                    secondary_up_to_date_t>(&kt->second.activity)) {
                            found = true;
                            break;
                        } else if (jt->second == blueprint_role_nothing &&
                                   boost::get<reactor_business_card_t::nothing_t>(
                                        &kt->second.activity)) {
                            found = true;
                            break;
                        } else {
                            ok = false;
                            return;
                        }
                    }
                }
                if (!found) {
                    ok = false;
                    return;
                }
            }
            ok = true;
        });
        if (!ok) {
            return false;
        }
    }
    return true;
}

class test_reactor_t : private ack_checker_t {
public:
    test_reactor_t(const base_path_t &base_path, io_backender_t *io_backender, reactor_test_cluster_t *r, const blueprint_t &initial_blueprint, multistore_ptr_t *svs);
    ~test_reactor_t();
    bool is_acceptable_ack_set(const std::set<server_id_t> &acks) const;
    write_durability_t get_write_durability() const {
        return write_durability_t::SOFT;
    }

    watchable_variable_t<blueprint_t> blueprint_watchable;
    backfill_throttler_t backfill_throttler;
    reactor_t reactor;
    scoped_ptr_t<directory_write_manager_t<namespace_directory_metadata_t> >
        directory_write_manager;
};



/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
reactor_test_cluster_t::reactor_test_cluster_t(int port) :
    connectivity_cluster(),
    mailbox_manager(&connectivity_cluster, 'M'),
    directory_read_manager(&connectivity_cluster, 'D'),
    connectivity_cluster_run(&connectivity_cluster,
                             get_unittest_addresses(),
                             peer_address_t(),
                             port,
                             0) { }

reactor_test_cluster_t::~reactor_test_cluster_t() { }

peer_id_t reactor_test_cluster_t::get_me() {
    return connectivity_cluster.get_me();
}

test_reactor_t::test_reactor_t(const base_path_t &base_path, io_backender_t *io_backender, reactor_test_cluster_t *r, const blueprint_t &initial_blueprint, multistore_ptr_t *svs) :
    blueprint_watchable(initial_blueprint),
    reactor(base_path, io_backender, &r->mailbox_manager, generate_uuid(),
            &backfill_throttler, this, r->directory_read_manager.get_root_map_view(),
            &r->branch_history_manager, blueprint_watchable.get_watchable(), svs,
            &get_global_perfmon_collection(), NULL),
    directory_write_manager(
        new directory_write_manager_t<namespace_directory_metadata_t>(
            &r->connectivity_cluster, 'D', reactor.get_reactor_directory())) {
    rassert(svs->get_region() == region_t::universe());
}

test_reactor_t::~test_reactor_t() { }

bool test_reactor_t::is_acceptable_ack_set(const std::set<server_id_t> &acks) const {
    return acks.size() >= 1;
}

test_cluster_group_t::test_cluster_group_t(int n_servers)
    : base_path("/tmp"), io_backender(new io_backender_t(file_direct_io_mode_t::buffered_desired)),
      balancer(new dummy_cache_balancer_t(GIGABYTE)) {
    for (int i = 0; i < n_servers; i++) {
        files.push_back(make_scoped<temp_file_t>());
        filepath_file_opener_t file_opener(files[i]->name(), io_backender.get());
        standard_serializer_t::create(&file_opener,
                                      standard_serializer_t::static_config_t());
        serializers.push_back(
                make_scoped<standard_serializer_t>(standard_serializer_t::dynamic_config_t(),
                                                   &file_opener,
                                                   &get_global_perfmon_collection()));
        stores.push_back(make_scoped<mock_store_t>(binary_blob_t(version_range_t(version_t::zero()))));
        store_view_t *store_ptr = stores[i].get();
        svses.push_back(make_scoped<multistore_ptr_t>(&store_ptr, 1));

        test_clusters.push_back(make_scoped<reactor_test_cluster_t>(ANY_PORT));
        if (i > 0) {
            test_clusters[0]->connectivity_cluster_run.join(
                get_cluster_local_address(&test_clusters[i]->connectivity_cluster));
        }
    }
}

test_cluster_group_t::~test_cluster_group_t() { }

void test_cluster_group_t::construct_all_reactors(const blueprint_t &bp) {
    for (size_t i = 0; i < test_clusters.size(); i++) {
        test_reactors.push_back(make_scoped<test_reactor_t>(base_path,
                                                            io_backender.get(),
                                                            test_clusters[i].get(),
                                                            bp,
                                                            svses[i].get()));
    }
}

peer_id_t test_cluster_group_t::get_peer_id(size_t i) {
    rassert(i < test_clusters.size());
    return test_clusters[i]->get_me();
}

blueprint_t test_cluster_group_t::compile_blueprint(const std::string &bp) {
    blueprint_t blueprint;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator tok_iterator;

    boost::char_separator<char> sep(",");
    tokenizer tokens(bp, sep);

    size_t peer = 0;
    for (tok_iterator it =  tokens.begin();
         it != tokens.end();
         it++) {

        blueprint.add_peer(get_peer_id(peer));
        for (size_t i = 0; i < it->size(); i++) {
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
    for (size_t i = 0; i < test_clusters.size(); i++) {
        test_reactors[i]->blueprint_watchable.set_value(bp);
    }
}

scoped_ptr_t<cluster_namespace_interface_t>
test_cluster_group_t::make_namespace_interface(int i) {
    std::map<namespace_id_t, std::map<key_range_t, server_id_t> > region_to_primary_maps;
    auto ret = make_scoped<cluster_namespace_interface_t>(
            &test_clusters[i]->mailbox_manager,
            &region_to_primary_maps,
            test_clusters[i]->directory_read_manager.get_root_map_view(),
            generate_uuid(),
            &ctx);
    ret->get_initial_ready_signal()->wait_lazily_unordered();
    return ret;
}

void test_cluster_group_t::run_queries() {
    nap(200);
    for (size_t i = 0; i < test_clusters.size(); i++) {
        scoped_ptr_t<cluster_namespace_interface_t> namespace_if
            = make_namespace_interface(i);

        order_source_t order_source;

        test_inserter_t inserter(namespace_if.get(), &dummy_key_gen, &order_source, "test_cluster_group_t::run_queries/inserter", &inserter_state);
        let_stuff_happen();
        inserter.stop();
        inserter.validate();
    }
}

void test_cluster_group_t::wait_until_blueprint_is_satisfied(const blueprint_t &bp) {
    try {
        const int timeout_ms = 60000;
        signal_timer_t timer;
        timer.start(timeout_ms);
        test_clusters[0]->directory_read_manager.get_root_map_view()
            ->run_all_until_satisfied(
                boost::bind(&is_blueprint_satisfied, bp, _1),
                &timer);
    } catch (const interrupted_exc_t &) {
        crash("The blueprint took too long to be satisfied, this is probably an error but you could try increasing the timeout.");
    }

    nap(100);
}

void test_cluster_group_t::wait_until_blueprint_is_satisfied(const std::string& bp) {
    wait_until_blueprint_is_satisfied(compile_blueprint(bp));
}

}  // namespace unittest

