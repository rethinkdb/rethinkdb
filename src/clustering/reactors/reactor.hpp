#ifndef __CLUSTERING_REACTORS_REACTOR_HPP__
#define __CLUSTERING_REACTORS_REACTOR_HPP__

#include "clustering/reactors/metadata.hpp"

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
    themselves when they no longer match `blueprint`.

    `pump()` never blocks. It is called at startup and in response to changes in
    `blueprint` or activities terminating. */
    void pump() THROWS_NOTHING;

    /* `pump()` spawns `do_role()` in a coroutine; `do_role()` calls
    `do_role_*()`. */
    void do_role(typename protocol_t::region_t region,
            typename blueprint_t<protocol_t>::role_t role,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    /* Returns `true` if any instance of `do_role()` is currently running in any
    region that overlaps `r`. It's important that no two instances of
    `do_role()` overlap each other. */
    bool region_is_active(typename protocol_t::region_t r) THROWS_NOTHING;

    /* `do_role_nothing()` runs for regions where we are not the primary or a
    secondary. If the blueprint is not completely implemented yet, it sets up a
    backfiller until the blueprint is complete, and then erases any data in the
    region and waits. */
    void do_role_nothing(typename protocol_t::region_t region, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    /* `do_role_primary()` runs for regions where we are supposed to be the
    primary. It backfills from a conveniently available backfiller and then
    sets up a broadcaster, master, listener, and replier. */
    void do_role_primary(typename protocol_t::region_t region, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    /* `do_role_secondary()` runs for regions where we are supposed to be a
    secondary. If a primary is available, it tries to track it. Otherwise, it
    offers backfills. */
    void do_role_secondary(typename protocol_t::region_t region, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    /* Sends messages to all peers asking them to confirm that their blueprint
    says we are supposed to take the given role for the given region. Returns
    `false` if any deny it or cannot be reached. */
    bool confirm_with_peers(typename protocol_t::region_t region,
            typename blueprint_t<protocol_t>::role_t role,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    void confirm_with_peer(peer_id_t *peer,
            typename protocol_t::region_t region,
            typename blueprint_t<protocol_t>::role_t role,
            signal_t *interruptor,
            bool *reply_out) THROWS_NOTHING;

    /* This is called when another peer requests confirmation. */
    void on_confirmation_request(typename protocol_t::region_t,
            peer_id_t peer,
            typename blueprint_t<protocol_t>::role_t,
            async_mailbox_t<void(bool)>::address_t) THROWS_NOTHING;

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > reactor_directory;
    boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history;
    watchable_t<blueprint_t<protocol_t> > *blueprint;

    typename protocol_t::store_file_t store_file;

    backfiller_t<protocol_t> backfiller;
    typename reactor_business_card_t<protocol_t>::confirmation_mailbox_t confirmation_mailbox;

    std::set<typename protocol_t::region_t> active_regions;

    /* `lock` protects `active_regions`. */
    mutex_assertion_t lock;

    auto_drainer_t drainer;

    watchable_subscription_t blueprint_subscription;
};

#endif /* __CLUSTERING_REACTORS_REACTOR_HPP__ */
