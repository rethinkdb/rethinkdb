// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "rpc/connectivity/peer_id.hpp"

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
public:
    class callback_t {
    public:
        virtual store_view_t::backfill_continue_t on_progress(
            const region_map_t<version_t> &chunk) = 0;
    };

    /* `backfillee_t()` blocks while it establishes a connection with the `backfiller_t`
    and does some other setup work, but not for the main duration of the backfill. */
    backfillee_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store,
        const backfiller_bcard_t &backfiller,
        signal_t *interruptor);

    /* Pulsing `interruptor` invalidates the `backfillee_t`. */
    void go(callback_t *callback, signal_t *interruptor);

private:
    class session_info_t {
    public:
        callback_t *callback;
        bool callback_returned_false;
        backfiller_bcard_t::session_id_t session_id;
        cond_t stop;
        new_mutex_t mutex;
        auto_drainer_t drainer;
    };

    void on_ack_pre_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        size_t chunk_size);

    void on_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        const session_id_t &session_id,
        const region_map_t<version_t> &version,
        const std::deque<backfill_chunk_t> &chunk);

    void send_pre_chunks(
        auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *const mailbox_manager;
    branch_history_manager_t *const branch_history_manager;
    store_view_t *const store;

    /* `completed_threshold` is the point that we've completely backfilled up to. */
    key_range_t::right_bound_t completed_threshold;

    session_info_t *current_session;

    fifo_source_t fifo_source;
    fifo_sink_t fifo_sink;

    /* `pre_atom_throttler` limits how many pre-atoms we send to the backfiller.
    `pre_atom_throttler_acq` always holds `pre_atom_throttler`, but its `count` changes
    to reflect how much capacity is currently owned by the other server. */
    new_semaphore_t pre_atom_throttler;
    new_semaphore_acq_t pre_atom_throttler_acq;

    backfiller_bcard_t::intro_2_t intro;

    auto_drainer_t drainer;
    backfiller_bcard_t::ack_pre_atoms_mailbox_t ack_pre_atoms_mailbox;
    backfiller_bcard_t::atoms_mailbox_t atoms_mailbox;
    scoped_ptr_t<registrant_t<backfiller_bcard_t::intro_1_t> > registrant;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_ */
