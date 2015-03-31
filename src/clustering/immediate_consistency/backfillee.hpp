// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_

#include "clustering/generic/registrant.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "concurrency/new_mutex.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "store_view.hpp"

/* `backfillee_t` is responsible for replicating data from a `backfiller_t` on some other
server to this server. Its interface is a bit complicated because the
`remote_replicator_client_t` needs to start applying writes before the backfill is
finished, so it needs to know when the `backfillee_t` reaches a certain point and also to
change the `backfillee_t`'s minimum version during the backfill. Here are the details:

- When `backfillee_t` is constructed, it takes ownership of the entire region covered by
    `store`. Nothing else is allowed to write to this region.
- Call `go()` to start the backfill.
- The backfill will proceed from left to right in lexicographical key order.
- The state that is backfilled is guaranteed to be at least as up-to-date as the state
    that the other server had at the time that `go()` was called.
- Whenever the backfill finishes processing a range of keys, it calls `on_progress()` on
    the callback, with a `region_t` representing that range.
- If `on_progress()` returns `CONTINUE` or `STOP_AFTER`, then all keys in that region are
    "released" by the `backfillee_t`, meaning that it will not write to that region, and
    other things are now allowed to write to that region. If `on_progress()` returns
    `STOP_BEFORE`, then the backfiller retains ownership of that region of keys.
- The backfill continues until `on_progress()` returns `STOP_*` or the backfill is
    completed. If the backfill is aborted early with `STOP_*`, the caller can call `go()`
    again to resume the backfill. The effect of this is to reset the minimum state that
    the backfill is guaranteed to go to; any keys backfilled during the new call to
    `go()` are guaranteed to be at least as up-to-date as the other server's state when
    the new call to `go()` happens.
*/

class backfillee_t : public home_thread_mixin_debug_only_t {
private:
    class session_t;

public:
    class callback_t {
    public:
        virtual bool on_progress(
            const region_map_t<version_t> &chunk) THROWS_ONLY(...) = 0;
    protected:
        virtual ~callback_t() { }
    };

    /* `backfillee_t()` blocks while it establishes a connection with the `backfiller_t`
    and does some other setup work, but not for the main duration of the backfill. */
    backfillee_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store,
        const backfiller_bcard_t &backfiller,
        signal_t *interruptor);
    ~backfillee_t();

    /* Pulsing `interruptor` invalidates the `backfillee_t`. */
    void go(callback_t *callback, const key_range_t::right_bound_t &start_point,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

private:
    void on_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        region_map_t<version_t> &&version,
        backfill_atom_seq_t<backfill_atom_t> &&chunk);

    void on_ack_end_session(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token);

    void send_pre_atoms(
        auto_drainer_t::lock_t keepalive);

    void on_ack_pre_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        size_t chunk_size);

    mailbox_manager_t *const mailbox_manager;
    branch_history_manager_t *const branch_history_manager;
    store_view_t *const store;

    backfiller_bcard_t::intro_2_t intro;

    fifo_enforcer_source_t fifo_source;
    fifo_enforcer_sink_t fifo_sink;

    /* `pre_atom_throttler` limits how many pre-atoms we send to the backfiller.
    `pre_atom_throttler_acq` always holds `pre_atom_throttler`, but its `count` changes
    to reflect how much capacity is currently owned by the other server. */
    new_semaphore_t pre_atom_throttler;
    new_semaphore_acq_t pre_atom_throttler_acq;

    scoped_ptr_t<session_t> current_session;

    auto_drainer_t drainer;
    backfiller_bcard_t::atoms_mailbox_t atoms_mailbox;
    backfiller_bcard_t::ack_end_session_mailbox_t ack_end_session_mailbox;
    backfiller_bcard_t::ack_pre_atoms_mailbox_t ack_pre_atoms_mailbox;
    scoped_ptr_t<registrant_t<backfiller_bcard_t::intro_1_t> > registrant;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_ */
