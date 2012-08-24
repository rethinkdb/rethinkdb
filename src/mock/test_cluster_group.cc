#include "mock/test_cluster_group.hpp"

#include <map>
#include <string>
#include <set>

#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "arch/io/io_utils.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "clustering/reactor/reactor.hpp"
#include "containers/archive/boost_types.hpp"
#include "concurrency/watchable.hpp"
#include "mock/branch_history_manager.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/dummy_protocol.hpp"
#include "mock/unittest_utils.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"

namespace mock {

template<class protocol_t>
class test_cluster_directory_t {
public:
    boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > reactor_directory;

    RDB_MAKE_ME_SERIALIZABLE_1(reactor_directory);
};


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
                    if (jt->second == blueprint_role_primary &&
                            boost::get<typename reactor_business_card_t<protocol_t>::primary_t>(&kt->second.second) &&
                            boost::get<typename reactor_business_card_t<protocol_t>::primary_t>(kt->second.second).replier.is_initialized()) {
                        found = true;
                        break;
                    } else if (jt->second == blueprint_role_secondary &&
                            boost::get<typename reactor_business_card_t<protocol_t>::secondary_up_to_date_t>(&kt->second.second)) {
                        found = true;
                        break;
                    } else if (jt->second == blueprint_role_nothing &&
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

    mock::in_memory_branch_history_manager_t<protocol_t> branch_history_manager;
};


template<class protocol_t>
class test_reactor_t : private master_t<protocol_t>::ack_checker_t {
public:
    test_reactor_t(io_backender_t *io_backender, reactor_test_cluster_t<protocol_t> *r, const blueprint_t<protocol_t> &initial_blueprint, multistore_ptr_t<protocol_t> *svs);
    ~test_reactor_t();
    bool is_acceptable_ack_set(const std::set<peer_id_t> &acks);

    watchable_variable_t<blueprint_t<protocol_t> > blueprint_watchable;
    reactor_t<protocol_t> reactor;
    field_copier_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >, test_cluster_directory_t<protocol_t> > reactor_directory_copier;

private:
    typename protocol_t::context_t ctx;
    static boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > wrap_in_optional(const directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > &input);

    static std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > extract_reactor_directory(const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &bcards);
};



/* This is a cluster that is useful for reactor testing... but doesn't actually
 * have a reactor due to the annoyance of needing the peer ids to create a
 * correct blueprint. */
template<class protocol_t>
reactor_test_cluster_t<protocol_t>::reactor_test_cluster_t(int port) :
    connectivity_cluster(),
    message_multiplexer(&connectivity_cluster),

    mailbox_manager_client(&message_multiplexer, 'M'),
    mailbox_manager(&mailbox_manager_client),
    mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),

    our_directory_variable(test_cluster_directory_t<protocol_t>()),
    directory_manager_client(&message_multiplexer, 'D'),
    directory_read_manager(&connectivity_cluster),
    directory_write_manager(&directory_manager_client, our_directory_variable.get_watchable()),
    directory_manager_client_run(&directory_manager_client, &directory_read_manager),

    message_multiplexer_run(&message_multiplexer),
    connectivity_cluster_run(&connectivity_cluster, port, &message_multiplexer_run) { }

template <class protocol_t>
reactor_test_cluster_t<protocol_t>::~reactor_test_cluster_t() { }

template <class protocol_t>
peer_id_t reactor_test_cluster_t<protocol_t>::get_me() {
    return connectivity_cluster.get_me();
}

template <class protocol_t>
test_reactor_t<protocol_t>::test_reactor_t(io_backender_t *io_backender, reactor_test_cluster_t<protocol_t> *r, const blueprint_t<protocol_t> &initial_blueprint, multistore_ptr_t<protocol_t> *svs) :
    blueprint_watchable(initial_blueprint),
    reactor(io_backender, &r->mailbox_manager, this,
            r->directory_read_manager.get_root_view()->subview(&test_reactor_t<protocol_t>::extract_reactor_directory),
            &r->branch_history_manager, blueprint_watchable.get_watchable(), svs, &get_global_perfmon_collection(), &ctx),
    reactor_directory_copier(&test_cluster_directory_t<protocol_t>::reactor_directory, reactor.get_reactor_directory()->subview(&test_reactor_t<protocol_t>::wrap_in_optional), &r->our_directory_variable) {
    rassert(svs->get_multistore_joined_region() == a_thru_z_region());
}

template <class protocol_t>
test_reactor_t<protocol_t>::~test_reactor_t() { }

template <class protocol_t>
bool test_reactor_t<protocol_t>::is_acceptable_ack_set(const std::set<peer_id_t> &acks) {
    return acks.size() >= 1;
}

template <class protocol_t>
boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > test_reactor_t<protocol_t>::wrap_in_optional(const directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > &input) {
    return boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >(input);
}

template <class protocol_t>
std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > test_reactor_t<protocol_t>::extract_reactor_directory(const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &bcards) {
    std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > out;
    for (typename std::map<peer_id_t, test_cluster_directory_t<protocol_t> >::const_iterator it = bcards.begin(); it != bcards.end(); it++) {
        out.insert(std::make_pair(it->first, it->second.reactor_directory));
    }
    return out;
}


template <class protocol_t>
test_cluster_group_t<protocol_t>::test_cluster_group_t(int n_machines) {
    int port = randport();
    make_io_backender(aio_default, &io_backender);

    for (int i = 0; i < n_machines; i++) {
        files.push_back(new temp_file_t("/tmp/rdb_unittest.XXXXXX"));
        stores.push_back(new typename protocol_t::store_t(io_backender.get(), files[i].name(), true, NULL, &ctx));
        store_view_t<protocol_t> *store_ptr = &stores[i];
        svses.push_back(new multistore_ptr_t<protocol_t>(&store_ptr, 1, &ctx));
        stores.back().metainfo.set(a_thru_z_region(), binary_blob_t(version_range_t(version_t::zero())));

        test_clusters.push_back(new reactor_test_cluster_t<protocol_t>(port + i));
        if (i > 0) {
            test_clusters[0].connectivity_cluster_run.join(test_clusters[i].connectivity_cluster.get_peer_address(test_clusters[i].connectivity_cluster.get_me()));
        }
    }
}

template <class protocol_t>
test_cluster_group_t<protocol_t>::~test_cluster_group_t() { }

template <class protocol_t>
void test_cluster_group_t<protocol_t>::construct_all_reactors(const blueprint_t<protocol_t> &bp) {
    for (unsigned i = 0; i < test_clusters.size(); i++) {
        test_reactors.push_back(new test_reactor_t<protocol_t>(io_backender.get(), &test_clusters[i], bp, &svses[i]));
    }
}

template <class protocol_t>
peer_id_t test_cluster_group_t<protocol_t>::get_peer_id(unsigned i) {
    rassert(i < test_clusters.size());
    return test_clusters[i].get_me();
}

template <class protocol_t>
blueprint_t<protocol_t> test_cluster_group_t<protocol_t>::compile_blueprint(const std::string& bp) {
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

template <class protocol_t>
void test_cluster_group_t<protocol_t>::set_all_blueprints(const blueprint_t<protocol_t> &bp) {
    for (unsigned i = 0; i < test_clusters.size(); i++) {
        test_reactors[i].blueprint_watchable.set_value(bp);
    }
}

template <class protocol_t>
std::map<peer_id_t, reactor_business_card_t<protocol_t> > test_cluster_group_t<protocol_t>::extract_reactor_business_cards_no_optional(
                                                                                                                                              const std::map<peer_id_t, test_cluster_directory_t<protocol_t> > &input) {
    std::map<peer_id_t, reactor_business_card_t<protocol_t> > out;
    for (typename std::map<peer_id_t, test_cluster_directory_t<protocol_t> >::const_iterator it = input.begin(); it != input.end(); it++) {
        if (it->second.reactor_directory) {
            out.insert(std::make_pair(it->first, it->second.reactor_directory->internal));
        } else {
            out.insert(std::make_pair(it->first, reactor_business_card_t<protocol_t>()));
        }
    }
    return out;
}

template <class protocol_t>
void test_cluster_group_t<protocol_t>::make_namespace_interface(int i, scoped_ptr_t<cluster_namespace_interface_t<protocol_t> > *out) {
    out->init(new cluster_namespace_interface_t<protocol_t>(
                                                            &test_clusters[i].mailbox_manager,
                                                            (&test_clusters[i])->directory_read_manager.get_root_view()
                                                            ->subview(&test_cluster_group_t::extract_reactor_business_cards_no_optional),
                                                            &ctx));
    (*out)->get_initial_ready_signal()->wait_lazily_unordered();
}

template <class protocol_t>
void test_cluster_group_t<protocol_t>::run_queries() {
    nap(200);
    for (unsigned i = 0; i < test_clusters.size(); i++) {
        scoped_ptr_t<cluster_namespace_interface_t<protocol_t> > namespace_if;
        make_namespace_interface(i, &namespace_if);

        order_source_t order_source;

        test_inserter_t inserter(namespace_if.get(), &key_gen<protocol_t>, &order_source, "test_cluster_group_t::run_queries/inserter", &inserter_state);
        let_stuff_happen();
        inserter.stop();
        inserter.validate();
    }
}

template <class protocol_t>
std::map<peer_id_t, boost::optional<reactor_business_card_t<protocol_t> > > test_cluster_group_t<protocol_t>::extract_reactor_business_cards(
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

template <class protocol_t>
void test_cluster_group_t<protocol_t>::wait_until_blueprint_is_satisfied(const blueprint_t<protocol_t> &bp) {
    try {
        const int timeout_ms = 60000;
        signal_timer_t timer(timeout_ms);
        test_clusters[0].directory_read_manager.get_root_view()
            ->subview(&test_cluster_group_t<protocol_t>::extract_reactor_business_cards)
            ->run_until_satisfied(boost::bind(&is_blueprint_satisfied<protocol_t>, bp, _1), &timer);
    } catch (interrupted_exc_t) {
        crash("The blueprint took too long to be satisfied, this is probably an error but you could try increasing the timeout.");
    }

    nap(100);
}

template <class protocol_t>
void test_cluster_group_t<protocol_t>::wait_until_blueprint_is_satisfied(const std::string& bp) {
    wait_until_blueprint_is_satisfied(compile_blueprint(bp));
}


template class test_cluster_group_t<mock::dummy_protocol_t>;

}   /* Namespace mock */
