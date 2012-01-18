#ifndef __CLUSTERING_REACTORS_DISPATCHER_HPP__
#define __CLUSTERING_REACTORS_DISPATCHER_HPP__

/* `manager_state_t` is the way that each peer tells peers what's currently
happening namespace on this machine. Each `manager_state_t` only applies to a
single namespace. */

template<class protocol_t>
class manager_state_t {
public:
    /* `inactive_shard_t` indicates that we have data for that shard, but we're
    not currently offering any services related to it. We are never in this
    state unless the cluster is broken or in the middle of a restructuring. */
    class inactive_shard_t {
    };

    /* `cold_shard_t` indicates that we have data for the shard and are offering
    it for backfill, but we aren't serving queries on it or tracking a live
    broadcaster. Like `inactive_shard_t`, we are never in this state unless the
    cluster is broken or in the middle of a restructuring. */
    class cold_shard_t {
    public:
        region_map_t<protocol_t, version_range_t> version;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* `primary_shard_t` indicates that we have a broadcaster, listener, and
    replier for the shard. */
    class primary_shard_t {
    public:
        branch_id_t branch_id;
        broadcaster_business_card_t<protocol_t> broadcaster;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* `secondary_shard_t` indicates that we have a listener and replier for the
    shard. */
    class secondary_shard_t {
    public:
        branch_id_t branch_id;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    typedef boost::variant<
            cold_shard_t,
            primary_shard_t,
            secondary_shard_t>
            shard_t;

    /* The keys in the `shards` map should never overlap */
    std::map<typename protocol_t::region_t, shard_t> shards;

    /* There is one more possible state, which is indicates by the absence of
    any entry in the `shards` map. This is used when we have no data for the
    region at all. */
};

template<class protocol_t>
class blueprint_t {
public:
    class shard_t {
        peer_id_t primary;
        std::set<peer_id_t> secondaries;
    };
    std::map<typename protocol_t::region_t, shard_t> shards;
};

template<class protocol_t>
class dispatcher_t {
public:
    dispatcher_t(
            mailbox_manager_t *mailbox_manager,
            clone_ptr_t<directory_readwrite_view_t<manager_state_t<protocol_t> > > manager_directory,
            boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
            watchable_t<blueprint_t<protocol_t> > *blueprint,
            std::string file_path) THROWS_NOTHING;

private:
    /* There are three "activity types": being a primary, being a secondary, and
    being neither. `activity_type_inconsistent` is used when we are assigned
    different activity types for different sub-regions of a region. */
    enum activity_type_t {
        activity_type_nothing,
        activity_type_primary,
        activity_type_secondary,
        activity_type_inconsistent
    };

    /* `activity_type_monitor_t` pulses itself if the blueprint no longer
    specifies that the activity for the given shard is the expected activity. */
    class activity_type_monitor_t : public signal_t {
    public:
        activity_type_monitor_t(dispatcher_t *parent, typename protocol_t::region_t region, activity_type_t expected);
    private:
        void on_change();
        bool is_failed();
        dispatcher_t *parent;
        typename protocol_t::region_t region;
        activity_type_t expected;
        watchable_subscription_t subscription;
    };

    /* Returns what activity the blueprint says we should be doing for the given
    region. */
    activity_type_t get_blueprint_activity(typename protocol_t::region_t region);

    /* `pump()` spawns activities and initializes/erases regions of `store_file`
    in an attempt to comply with `blueprint`. It doesn't terminate activities;
    activities are responsible for terminating themselves when they no longer
    match `blueprint`.

    `pump()` never blocks. It is called at startup and in response to changes in
    `blueprint` or activities terminating. */
    void pump() THROWS_NOTHING;

    /* `spawn_activity()` marks the given region as active and then spawns the
    function in a coroutine. When the function finishes, it marks the region as
    inactive and then calls `pump()`. It returns immediately after spawning the
    coroutine. */
    void spawn_activity(typename protocol_t::region_t region,
            const boost::function<void(auto_drainer_t::lock_t keepalive)> &activity,
            mutex_assertion_t::acq_t *proof) THROWS_NOTHING;

    /* `spawn_activity()` spawns `do_activity()` as a wrapper around the
    activity. */
    void do_activity(typename protocol_t::region_t region,
            const boost::function<void(auto_drainer_t::lock_t keepalive)> &activity,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* Returns `true` if any activity is currently running in any region that
    overlaps `r`. It's important that no activities overlap each other. */
    bool region_is_active(typename protocol_t::region_t r) THROWS_NOTHING;

    /* `activity_*()` are all spawned in coroutines by `spawn_activity()`. */

    /* `activity_nothing()` runs for regions where we are not the primary or a
    secondary. If the blueprint is not completely implemented yet, it sets up a
    backfiller until the blueprint is complete, and then erases any data in the
    region and waits. */
    void activity_nothing(typename protocol_t::region_t region, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* `activity_primary()` runs for regions where we are supposed to be the
    primary. It backfills from a conveniently available backfiller and then
    sets up a broadcaster, master, listener, and replier. */
    void activity_primary(typename protocol_t::region_t region, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* `activity_secondary()` runs for regions where we are supposed to be a
    secondary. If a primary is available, it tries to track it. Otherwise, it
    offers backfills. */
    void activity_secondary(typename protocol_t::region_t region, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_readwrite_view_t<manager_state_t<protocol_t> > > manager_directory;
    boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history;
    watchable_t<blueprint_t<protocol_t> > *blueprint;

    typename protocol_t::store_file_t store_file;

    backfiller_t<protocol_t> backfiller;

    std::set<typename protocol_t::region_t> active_regions;

    /* `lock` protects `active_regions`. */
    mutex_assertion_t lock;

    auto_drainer_t drainer;

    watchable_subscription_t blueprint_subscription;
};

#endif /* __CLUSTERING_REACTORS_DISPATCHER_HPP__ */
