// Copyright 2010-2014 RethinkDB, all rights reserved.
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
#include "containers/cow_ptr.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/semilattice/view.hpp"

class io_backender_t;
class multistore_ptr_t;
class backfill_throttler_t;

/* `reactor_t::get_progress()` will return a map with one `reactor_progress_report_t` for
each primary or secondary shard that is currently backfilling or has completed a
backfill. */
class reactor_progress_report_t {
public:
    /* `is_ready` is true if the shard is completely ready. */
    bool is_ready;

    /* `start_time` is the moment the `reactor_progress_report_t` was constructed. */
    microtime_t start_time;

    /* `backfills` contains the peer ID and progress fraction (from 0 to 1) for each
    backfill that this server is receiving for that shard. The backfills will stay even
    after `is_ready` becomes `true`, but all the progress fractions will be 1.  */
    std::vector<std::pair<peer_id_t, double> > backfills;
};

class reactor_t : public home_thread_mixin_t {
public:
    reactor_t(
            const base_path_t& base_path,
            io_backender_t *io_backender,
            mailbox_manager_t *mailbox_manager,
            const server_id_t &server_id,
            backfill_throttler_t *backfill_throttler,
            ack_checker_t *ack_checker,
            watchable_map_t<
                peer_id_t,
                directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> >
                > *reactor_directory,
            branch_history_manager_t *branch_history_manager,
            clone_ptr_t<watchable_t<blueprint_t> > blueprint_watchable,
            multistore_ptr_t *_underlying_svs,
            perfmon_collection_t *_parent_perfmon_collection,
            rdb_context_t *) THROWS_NOTHING;

    clone_ptr_t<watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > > get_reactor_directory();

    /* This might block */
    std::map<region_t, reactor_progress_report_t> get_progress();

private:
    /* a directory_entry_t is a sentry that in its contructor inserts an entry
     * into the directory for a role that we are performing (a role that we
     * have spawned a coroutine to perform). In its destructor it removes that
     * entry.
     */
    class directory_entry_t {
    public:
        directory_entry_t(reactor_t *, region_t);

        //Changes just the activity, leaves everything else in tact
        directory_echo_version_t set(reactor_business_card_t::activity_t);

        //XXX this is a bit of a hack that we should revisit when we know more
        directory_echo_version_t update_without_changing_id(reactor_business_card_t::activity_t);
        ~directory_entry_t();

        reactor_activity_id_t get_reactor_activity_id() const;
    private:
        reactor_t *const parent;
        const region_t region;
        reactor_activity_id_t reactor_activity_id;

        DISABLE_COPYING(directory_entry_t);
    };

    class current_role_t {
    public:
        current_role_t(blueprint_role_t r, const blueprint_t &b);
        blueprint_role_t role;
        watchable_variable_t<blueprint_t> blueprint;
        cond_t abort_roles;
    };

    /* To save typing */
    peer_id_t get_me() THROWS_NOTHING;


    void on_blueprint_changed() THROWS_NOTHING;
    void try_spawn_roles() THROWS_NOTHING;
    void run_cpu_sharded_role(
            int cpu_shard_number,
            current_role_t *role,
            const region_t& region,
            multistore_ptr_t *svs_subview,
            signal_t *interruptor,
            cond_t *abort_roles) THROWS_NOTHING;
    void run_role(
            region_t region,
            current_role_t *role,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* Implemented in clustering/reactor/reactor_be_primary.tcc */
    void be_primary(region_t region, store_view_t *store, const clone_ptr_t<watchable_t<blueprint_t> > &,
            signal_t *interruptor) THROWS_NOTHING;

    /* A backfill candidate is a structure we use to keep track of the different
     * peers we could backfill from and what we could backfill from them to make it
     * easier to grab data from them. */
    class backfill_candidate_t {
    public:
        version_range_t version_range;
        typedef clone_ptr_t<watchable_t<boost::optional<boost::optional<backfiller_business_card_t> > > > backfiller_bcard_view_t;
        class backfill_location_t {
        public:
            backfill_location_t(const backfiller_bcard_view_t &b, peer_id_t p, reactor_activity_id_t i);
            backfiller_bcard_view_t backfiller;
            peer_id_t peer_id;
            reactor_activity_id_t activity_id;
        };

        std::vector<backfill_location_t> places_to_get_this_version;
        bool present_in_our_store;

        backfill_candidate_t(version_range_t _version_range, std::vector<backfill_location_t> _places_to_get_this_version, bool _present_in_our_store);
    };

    typedef region_map_t<backfill_candidate_t> best_backfiller_map_t;

    void update_best_backfiller(const region_map_t<version_range_t> &offered_backfill_versions,
                                const backfill_candidate_t::backfill_location_t &backfiller,
                                best_backfiller_map_t *best_backfiller_out);

    bool is_safe_for_us_to_be_primary(
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        const blueprint_t &blueprint,
        const region_t &region,
        best_backfiller_map_t *best_backfiller_out,
        branch_history_t *branch_history_to_merge_out,
        bool *should_merge_metadata);

    void is_safe_for_us_to_be_primary_helper(
        const peer_id_t &peer,
        const reactor_business_card_t &bcard,
        const region_t &region,
        best_backfiller_map_t *best_backfiller_out,
        branch_history_t *branch_history_to_merge_out,
        bool *merge_branch_history_out,
        bool *its_not_safe_out);

    static backfill_candidate_t make_backfill_candidate_from_version_range(const version_range_t &b);

    /* Implemented in clustering/reactor/reactor_be_secondary.tcc */
    bool find_broadcaster_in_directory(
        const region_t &region,
        const blueprint_t &bp,
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<
            broadcaster_business_card_t> > > > *broadcaster_out);

    bool find_replier_in_directory(
        const region_t &region,
        const branch_id_t &b_id,
        const blueprint_t &bp,
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        clone_ptr_t<watchable_t<boost::optional<boost::optional<
            replier_business_card_t> > > > *replier_out,
        peer_id_t *peer_id_out,
        reactor_activity_id_t *activity_out);

    void be_secondary(region_t region, store_view_t *store, const clone_ptr_t<watchable_t<blueprint_t> > &,
            signal_t *interruptor) THROWS_NOTHING;


    /* Implemented in clustering/reactor/reactor_be_nothing.tcc */
    bool is_safe_for_us_to_be_nothing(
        watchable_map_t<peer_id_t, cow_ptr_t<reactor_business_card_t> > *directory,
        const blueprint_t &blueprint,
        const region_t &region);

    void be_nothing(region_t region, store_view_t *store, const clone_ptr_t<watchable_t<blueprint_t> > &,
            signal_t *interruptor) THROWS_NOTHING;

    static boost::optional<boost::optional<broadcaster_business_card_t> > extract_broadcaster_from_reactor_business_card_primary(
        const boost::optional<boost::optional<reactor_business_card_t::primary_t> > &bcard);

    /* Shared between all three roles (primary, secondary, nothing) */

    void wait_for_directory_acks(directory_echo_version_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    bool attempt_backfill_from_peers(
        directory_entry_t *directory_entry,
        reactor_progress_report_t *progress_tracker_on_svs_thread,
        order_source_t *order_source,
        const region_t &region,
        store_view_t *svs,
        const clone_ptr_t<watchable_t<blueprint_t> > &blueprint,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    template <class activity_t>
    clone_ptr_t<watchable_t<boost::optional<boost::optional<activity_t> > > > get_directory_entry_view(peer_id_t id, const reactor_activity_id_t&);

    const base_path_t base_path;
    perfmon_collection_t *parent_perfmon_collection;
    perfmon_collection_t regions_perfmon_collection;
    perfmon_membership_t regions_perfmon_membership;

    io_backender_t *io_backender;

    mailbox_manager_t *mailbox_manager;

    server_id_t server_id;

    backfill_throttler_t *backfill_throttler;

    ack_checker_t *ack_checker;

    directory_echo_writer_t<cow_ptr_t<reactor_business_card_t> > directory_echo_writer;
    watchable_buffer_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >
        directory_buffer;
    directory_echo_mirror_t<cow_ptr_t<reactor_business_card_t> > directory_echo_mirror;
    branch_history_manager_t *branch_history_manager;

    clone_ptr_t<watchable_t<blueprint_t> > blueprint_watchable;

    multistore_ptr_t *underlying_svs;

    std::map<region_t, current_role_t *> current_roles;

    /* `reactor_be_primary()` and `reactor_be_secondary()` automatically maintain their
    own entries in this map. Using `one_per_thread_t` is kind of overkill here because
    it automatically switches threads to do the construction and destruction, but the
    additional overhead from the thread switches isn't a big enough cost to justify
    changing it. */
    one_per_thread_t<std::map<region_t, reactor_progress_report_t> > progress_map;

    auto_drainer_t drainer;

    watchable_t<blueprint_t>::subscription_t blueprint_subscription;
    rdb_context_t *ctx;

    DISABLE_COPYING(reactor_t);
};

template <class activity_t>
clone_ptr_t<watchable_t<boost::optional<boost::optional<activity_t> > > > reactor_t::get_directory_entry_view(peer_id_t p_id, const reactor_activity_id_t &ra_id) {
    return get_watchable_for_key(directory_echo_mirror.get_internal(), p_id)->subview(
        [this, p_id, ra_id]
        (const boost::optional<cow_ptr_t<reactor_business_card_t> > &bcard)
        -> boost::optional<boost::optional<activity_t> > {
            if (!static_cast<bool>(bcard)) {
                return boost::optional<boost::optional<activity_t> >();
            }
            reactor_business_card_t::activity_map_t::const_iterator jt =
                (*bcard)->activities.find(ra_id);
            if (jt == (*bcard)->activities.end()) {
                return boost::optional<boost::optional<activity_t> >(
                    boost::optional<activity_t>());
            }
            try {
                return boost::optional<boost::optional<activity_t> >(
                    boost::optional<activity_t>(
                        boost::get<activity_t>(jt->second.activity)));
            } catch (const boost::bad_get &) {
                crash("Tried to get an activity of an unexpected type! It is assumed "
                    "the person calling this function knows the type of the activity "
                    "they will be getting back.\n");
            }
        });
}

/* `run_until_satisfied_2` repeatedly calls the given function on the contents of the
given `watchable_map_t` and `watchable_t` until the function returns `true` or the
interruptor is pulsed. It's efficient because it only calls the function when the values
of the watchables change. */
template<class key_t, class value_t, class value2_t, class callable_t>
void run_until_satisfied_2(
        watchable_map_t<key_t, value_t> *input1,
        clone_ptr_t<watchable_t<value2_t> > input2,
        const callable_t &fun,
        signal_t *interruptor,
        int64_t nap_before_retry_ms = 0) {
    cond_t *notify = nullptr;
    typename watchable_map_t<key_t, value_t>::all_subs_t all_subs(
        input1,
        [&notify](const key_t &, const value_t *) {
            if (notify != nullptr) {
                notify->pulse_if_not_already_pulsed();
            }
        },
        false);
    typename watchable_t<value2_t>::subscription_t subs(
        [&notify]() {
            if (notify != nullptr) {
                notify->pulse_if_not_already_pulsed();
            }
        });
    {
        typename watchable_t<value2_t>::freeze_t freeze(input2);
        subs.reset(input2, &freeze);
    }
    while (true) {
        cond_t cond;
        assignment_sentry_t<cond_t *> sentry(&notify, &cond);
        bool ok;
        input2->apply_read(
            [&](const value2_t *value2) {
                ok = fun(input1, *value2);
            });
        if (ok) {
            return;
        }
        signal_timer_t timeout;
        timeout.start(nap_before_retry_ms);
        wait_interruptible(&timeout, interruptor);
        wait_interruptible(&cond, interruptor);
    }
}

#endif /* CLUSTERING_REACTOR_REACTOR_HPP_ */

