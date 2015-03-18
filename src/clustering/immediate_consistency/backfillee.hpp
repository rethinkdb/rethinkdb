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
- If `on_progress()` returns `true`, then all keys in that region are "released" by
    the `backfillee_t`, meaning that it will not write to that region, and other things
    are now allowed to write to that region.
- If `on_progress()` returns `false`, then the backfiller retains ownership of that
    region of keys, and `go()` returns `false`.
- The caller can call `go()` again to resume the backfill. The effect of this is to reset
    the minimum state that the backfill is guaranteed to go to; any keys backfilled
    during the new call to `go()` are guaranteed to be at least as up-to-date as the
    other server's state when the new call to `go()` happens. This also applies to keys
    that were covered by the call to `on_progress()` that returned `false`.
- If the `backfillee_t` reaches the end of the store's region and the last call to
    `on_progress()` returns `true`, then `go()` returns `true`.
*/

class backfillee_t : public home_thread_mixin_debug_only_t {
public:
    class callback_t {
    public:
        virtual bool on_progress(const region_map_t<version_t> &chunk) = 0;
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
    bool go(callback_t *callback, signal_t *interruptor);

private:
    /* `on_chunk()` is the mailbox callback for a mailbox that `go()` constructs.
    `fifo_token`, `data`, and `version` are mailbox message parts; `callback`,
    `callback_mutex`, `stop_cond`, and `callback_returned_false_out` are set by `go()`.
    */
    void on_chunk(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        const std::deque<backfill_chunk_t> &data,
        const region_map_t<version_t> &version,
        callback_t *callback,
        new_mutex_t *callback_mutex,
        cond_t *stop_cond,
        bool *callback_returned_false_out);

    void send_pre_chunks(
        signal_t *interruptor);

    mailbox_manager_t *const mailbox_manager;
    branch_history_manager_t *const branch_history_manager;
    store_view_t *const store;

    /* This is the timestamps of the common ancestor of the current store state and the
    `backfiller_t`'s store state at the time that the `backfillee_t` is created. */
    region_map_t<state_timestamp_t> common_ancestor;

    key_range_t to_backfill;
    key_range_t to_send_pre_chunks;

    fifo_source_t fifo_source;
    fifo_sink_t fifo_sink;

    scoped_ptr_t<registrant_t<backfiller_bcard_t::intro_1_t> > registrant;
    backfiller_bcard_t::intro2_t intro;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_ */
