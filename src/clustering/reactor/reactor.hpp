#ifndef __CLUSTERING_REACTOR_REACTOR_HPP__
#define __CLUSTERING_REACTOR_REACTOR_HPP__

#include "clustering/reactors/metadata.hpp"
#include "clustering/reactors/delayed_pump.hpp"

template<class protocol_t>
class reactor_t {
public:
    reactor_t(
            mailbox_manager_t *mailbox_manager,
            clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > reactor_directory,
            boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
            watchable_t<blueprint_t<protocol_t> > *blueprint,
            std::string file_path) THROWS_NOTHING;

private:
    /* `role_monitor_t` pulses itself if the blueprint no longer assigns us the
    expected role for the given shard. */
    class role_monitor_t : public signal_t {
    public:
        role_monitor_t(reactor_t *parent, typename protocol_t::region_t region, typename blueprint_t<protocol_t>::role_t expected);
    private:
        void on_change();
        bool is_failed();
        reactor_t *parent;
        typename protocol_t::region_t region;
        typename blueprint_t<protocol_t>::role_t expected;
        watchable_subscription_t subscription;
    };

    /* `pump()` spawns activities in an attempt to comply with `blueprint`. It
    doesn't terminate activities; activities are responsible for terminating
    themselves when they no longer match `blueprint`. Don't call it directly;
    instead, call `need_to_pump()`. */ 
    void pump(auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* `schedule_pump()` makes sure that `pump()` gets called in the near
    future. It does not block. */
    void schedule_pump() THROWS_NOTHING;

    /* `spawn_activity()` spawns `run_activity()` in a coroutine. It also takes
    care of recording that the activity is running in `active_regions`. */
    void spawn_activity(typename protocol_t::region_t region,
            const boost::function<void(store_view_t *, signal_t *)> &activity,
            mutex_assertion_t *proof_of_activity_mutex) THROWS_NOTHING;
    void run_activity(typename protocol_t::region_t region,
            const boost::function<void(store_view_t *, signal_t *)> &activity,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* Returns `true` if any instance of `do_role()` is currently running in any
    region that overlaps `r`. It's important that no two instances of
    `do_role()` overlap each other. */
    bool region_is_active(typename protocol_t::region_t r) THROWS_NOTHING;

    void reset_region(typename protocol_t::region_t region, store_view_t *sub_store, signal_t *interruptor) THROWS_NOTHING;
    void be_primary(typename protocol_t::region_t region, store_view_t *sub_store, signal_t *interruptor) THROWS_NOTHING;
    void be_secondary(typename protocol_t::region_t region, store_view_t *sub_store, signal_t *interruptor) THROWS_NOTHING;
    void backfill_in(typename protocol_t::region_t region, store_view_t *sub_store, clone_ptr_t<directory_single_rview_t<backfiller_business_card_t<protocol_t> > > backfiller, signal_t *interruptor) THROWS_NOTHING;
    void offer_backfills(typename protocol_t::region_t region, store_view_t *sub_store, signal_t *interruptor) THROWS_NOTHING;

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > reactor_directory;
    boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history;
    watchable_t<blueprint_t<protocol_t> > *blueprint;

    typename protocol_t::store_file_t store_file;

    backfiller_t<protocol_t> backfiller;

    delayed_pump_t pumper;

    std::set<typename protocol_t::region_t> active_regions;

    /* `lock` protects `active_regions`. */
    mutex_assertion_t lock;

    auto_drainer_t drainer;

    watchable_subscription_t blueprint_subscription;
    directory_read_service_t::whole_directory_subscription_t directory_subscription;
};

#endif /* __CLUSTERING_REACTOR_REACTOR_HPP__ */
