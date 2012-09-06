#ifndef CLUSTERING_REACTOR_REACTOR_HPP_
#define CLUSTERING_REACTOR_REACTOR_HPP_

#include <map>
#include <vector>

#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/reactor/metadata.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/semilattice/view.hpp"

// This "must" be hard-coded because a cluster cannot run with
// differing cpu sharding factors.
// TODO: Make this not be 2.
const int CLUSTER_CPU_SHARDING_FACTOR = 2;

class io_backender_t;
template <class> class multistore_ptr_t;

template<class protocol_t>
class reactor_t : public home_thread_mixin_t {
public:
    reactor_t(
            io_backender_t *io_backender,
            mailbox_manager_t *mailbox_manager,
            typename master_t<protocol_t>::ack_checker_t *ack_checker,
            clone_ptr_t<watchable_t<std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > > reactor_directory,
            branch_history_manager_t<protocol_t> *branch_history_manager,
            clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > blueprint_watchable,
            multistore_ptr_t<protocol_t> *_underlying_svs,
            perfmon_collection_t *_parent_perfmon_collection,
            typename protocol_t::context_t *) THROWS_NOTHING;

    clone_ptr_t<watchable_t<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > get_reactor_directory() {
        return directory_echo_writer.get_watchable();
    }

private:
    /* a directory_entry_t is a sentry that in its contructor inserts an entry
     * into the directory for a role that we are performing (a role that we
     * have spawned a coroutine to perform). In its destructor it removes that
     * entry.
     */
    class directory_entry_t {
    public:
        directory_entry_t(reactor_t<protocol_t> *, typename protocol_t::region_t);

        //Changes just the activity, leaves everything else in tact
        directory_echo_version_t set(typename reactor_business_card_t<protocol_t>::activity_t);

        //XXX this is a bit of a hack that we should revisit when we know more
        directory_echo_version_t update_without_changing_id(typename reactor_business_card_t<protocol_t>::activity_t);
        ~directory_entry_t();

        reactor_activity_id_t get_reactor_activity_id() const {
            return reactor_activity_id;
        }
    private:
        reactor_t<protocol_t> *const parent;
        const typename protocol_t::region_t region;
        reactor_activity_id_t reactor_activity_id;

        DISABLE_COPYING(directory_entry_t);
    };

    class current_role_t {
    public:
        current_role_t(blueprint_role_t r, const blueprint_t<protocol_t> &b) :
            role(r), blueprint(b) { }
        blueprint_role_t role;
        watchable_variable_t<blueprint_t<protocol_t> > blueprint;
        cond_t abort;
    };

    /* To save typing */
    peer_id_t get_me() THROWS_NOTHING {
        return mailbox_manager->get_connectivity_service()->get_me();
    }

    void on_blueprint_changed() THROWS_NOTHING;
    void try_spawn_roles() THROWS_NOTHING;
    void run_cpu_sharded_role(
            int cpu_shard_number,
            current_role_t *role,
            const typename protocol_t::region_t& region,
            multistore_ptr_t<protocol_t> *svs_subview,
            signal_t *interruptor) THROWS_NOTHING;
    void run_role(
            typename protocol_t::region_t region,
            current_role_t *role,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* Implemented in clustering/reactor/reactor_be_primary.tcc */
    void be_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &,
            signal_t *interruptor) THROWS_NOTHING;

    /* A backfill candidate is a structure we use to keep track of the different
     * peers we could backfill from and what we could backfill from them to make it
     * easier to grab data from them. */
    class backfill_candidate_t {
    public:
        version_range_t version_range;
        typedef clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > > > backfiller_bcard_view_t;
        class backfill_location_t {
        public:
            backfill_location_t(const backfiller_bcard_view_t &b, peer_id_t p, reactor_activity_id_t i) :
                 backfiller(b), peer_id(p), activity_id(i) { }
            backfiller_bcard_view_t backfiller;
            peer_id_t peer_id;
            reactor_activity_id_t activity_id;
        };

        std::vector<backfill_location_t> places_to_get_this_version;
        bool present_in_our_store;

        backfill_candidate_t(version_range_t _version_range, std::vector<backfill_location_t> _places_to_get_this_version, bool _present_in_our_store);
    };

    typedef region_map_t<protocol_t, backfill_candidate_t> best_backfiller_map_t;

    void update_best_backfiller(const region_map_t<protocol_t, version_range_t> &offered_backfill_versions,
                                const typename backfill_candidate_t::backfill_location_t &backfiller,
                                best_backfiller_map_t *best_backfiller_out);

    bool is_safe_for_us_to_be_primary(const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &reactor_directory, const blueprint_t<protocol_t> &blueprint,
                                      const typename protocol_t::region_t &region, best_backfiller_map_t *best_backfiller_out);

    static backfill_candidate_t make_backfill_candidate_from_version_range(const version_range_t &b);

    /* Implemented in clustering/reactor/reactor_be_secondary.tcc */
    bool find_broadcaster_in_directory(const typename protocol_t::region_t &region, const blueprint_t<protocol_t> &bp, const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &reactor_directory,
                                       clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > *broadcaster_out);

    bool find_replier_in_directory(const typename protocol_t::region_t &region, const branch_id_t &b_id, const blueprint_t<protocol_t> &bp, const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &reactor_directory,
                                      clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > *replier_out, peer_id_t *peer_id_out, reactor_activity_id_t *activity_out);

    void be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &,
            signal_t *interruptor) THROWS_NOTHING;


    /* Implemented in clustering/reactor/reactor_be_nothing.tcc */
    bool is_safe_for_us_to_be_nothing(const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &reactor_directory, const blueprint_t<protocol_t> &blueprint,
                                      const typename protocol_t::region_t &region);

    void be_nothing(typename protocol_t::region_t region, store_view_t<protocol_t> *store, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &,
            signal_t *interruptor) THROWS_NOTHING;

    static boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > extract_broadcaster_from_reactor_business_card_primary(
        const boost::optional<boost::optional<typename reactor_business_card_t<protocol_t>::primary_t> > &bcard);

    /* Shared between all three roles (primary, secondary, nothing) */

    void wait_for_directory_acks(directory_echo_version_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    bool attempt_backfill_from_peers(directory_entry_t *directory_entry, order_source_t *order_source, const typename protocol_t::region_t &region, store_view_t<protocol_t> *svs, const clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > &blueprint, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    template <class activity_t>
    clone_ptr_t<watchable_t<boost::optional<boost::optional<activity_t> > > > get_directory_entry_view(peer_id_t id, const reactor_activity_id_t&);

    io_backender_t *io_backender;

    mailbox_manager_t *mailbox_manager;

    typename master_t<protocol_t>::ack_checker_t *ack_checker;

    clone_ptr_t<watchable_t<std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > > reactor_directory;
    directory_echo_writer_t<reactor_business_card_t<protocol_t> > directory_echo_writer;
    directory_echo_mirror_t<reactor_business_card_t<protocol_t> > directory_echo_mirror;
    branch_history_manager_t<protocol_t> *branch_history_manager;

    clone_ptr_t<watchable_t<blueprint_t<protocol_t> > > blueprint_watchable;

    multistore_ptr_t<protocol_t> *underlying_svs;

    std::map<typename protocol_t::region_t, current_role_t *> current_roles;

    auto_drainer_t drainer;

    typename watchable_t<blueprint_t<protocol_t> >::subscription_t blueprint_subscription;

    perfmon_collection_t *parent_perfmon_collection;
    perfmon_collection_t regions_perfmon_collection;
    perfmon_membership_t regions_perfmon_membership;

    typename protocol_t::context_t *ctx;

    DISABLE_COPYING(reactor_t);
};


template<class protocol_t, class activity_t>
boost::optional<boost::optional<activity_t> > extract_activity_from_reactor_bcard(const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &bcards, peer_id_t p_id, const reactor_activity_id_t &ra_id) {
    typename std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > >::const_iterator it = bcards.find(p_id);
    if (it == bcards.end()) {
        return boost::optional<boost::optional<activity_t> >();
    }
    if (!it->second) {
        return boost::optional<boost::optional<activity_t> >(boost::optional<activity_t>());
    }
    typename reactor_business_card_t<protocol_t>::activity_map_t::const_iterator jt = it->second->internal.activities.find(ra_id);
    if (jt == it->second->internal.activities.end()) {
        return boost::optional<boost::optional<activity_t> >(boost::optional<activity_t>());
    }
    try {
        return boost::optional<boost::optional<activity_t> >(boost::optional<activity_t>(boost::get<activity_t>(jt->second.second)));
    } catch (boost::bad_get) {
        crash("Tried to get an activity of an unexpected type, it is assumed "
            "the person calling this function knows the type of the activity "
            "they will be getting back.\n");
    }
}

template <class protocol_t>
template <class activity_t>
clone_ptr_t<watchable_t<boost::optional<boost::optional<activity_t> > > > reactor_t<protocol_t>::get_directory_entry_view(peer_id_t p_id, const reactor_activity_id_t &ra_id) {
    return reactor_directory->subview(boost::bind(&extract_activity_from_reactor_bcard<protocol_t, activity_t>, _1, p_id, ra_id));
}


#endif /* CLUSTERING_REACTOR_REACTOR_HPP_ */

