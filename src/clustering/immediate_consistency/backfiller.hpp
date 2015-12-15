// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_

#include <map>
#include <utility>

#include "clustering/generic/registrar.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "store_view.hpp"

class backfill_progress_tracker_t;

/* `backfiller_t` is responsible for copying the given store's state to other servers via
`backfillee_t`.

It assumes that if the state of the underlying store changes, the only change will be to
apply writes along the current branch. In particular, it might break if the underlying
store receives a backfill, changes branches, or erases data while the `backfiller_t`
exists. (If the underlying store is a `store_subview_t`, it's OK if other changes happen
to the underlying store's underlying store outside of the region covered by the
`store_subview_t`.) */

class backfiller_t : public home_thread_mixin_debug_only_t {
public:
    backfiller_t(mailbox_manager_t *_mailbox_manager,
                 branch_history_manager_t *_branch_history_manager,
                 store_view_t *_store);

    backfiller_bcard_t get_business_card() {
        return backfiller_bcard_t {
            store->get_region(),
            registrar.get_business_card() };
    }

private:
    /* A `client_t` is created for every backfill that's in progress. */
    class client_t {
    public:
        client_t(backfiller_t *, const backfiller_bcard_t::intro_1_t &intro, signal_t *);

    private:
        /* `session_t` represents a session within this backfill; the definition is
        hidden to keep this header file simple. */
        class session_t;

        void on_begin_session(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token,
            const key_range_t::right_bound_t &threshold);
        void on_end_session(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token);
        void on_ack_items(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token,
            size_t mem_size);
        void on_pre_items(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token,
            backfill_item_seq_t<backfill_pre_item_t> &&chunk);

        backfiller_t *const parent;

        /* `intro` is what we received from the backfillee in the initial handshake. */
        backfiller_bcard_t::intro_1_t const intro;

        /* `full_region` is the entire region that this backfill applies to. It's the
        same as `intro.initial_version.get_domain()`. */
        region_t const full_region;

        /* `fifo_source` is used to attach order tokens to the messages we send to the
        backfillee. `fifo_sink` is used to interpret the order tokens on messages we
        receive from the backfillee. */
        fifo_enforcer_source_t fifo_source;
        fifo_enforcer_sink_t fifo_sink;

        /* `common_version` and `pre_items` together describe the state of the
        backfillee's B-tree relative to ours. Initially, `common_version` is set to the
        timestamp of the common ancestor of our B-tree's version and the backfillee's
        B-tree's version, and `pre_items` contains backfill pre items describing which
        keys on the backfillee have changed since the common ancestor (*see note). When a
        backfill session covers a certain region for the first time, it consumes the
        corresponding part of `pre_items` and updates `common_version` to be whatever
        version it sends to the backfillee. So if a subsequent backfill session backfills
        the same region, then it only backfills things that have changed since the first
        backfill session.

        * Note: Actually, `pre_items` is initially empty. As we stream pre-items from
        the backfillee, they are appended to the right-hand side. So it conceptually
        contains all the pre items, but in reality it's filled in incrementally. */
        region_map_t<state_timestamp_t> common_version;
        backfill_item_seq_t<backfill_pre_item_t> pre_items;

        /* `item_throttler` limits the total mem size of the backfill items that are
        allowed to queue up on the backfillee. `item_throttler_acq` always holds
        `item_throttler`, but its `count` changes to reflect the total mem size that's
        currently in the queue. */
        new_semaphore_t item_throttler;
        new_semaphore_in_line_t item_throttler_acq;

        scoped_ptr_t<session_t> current_session;

        backfiller_bcard_t::pre_items_mailbox_t pre_items_mailbox;
        backfiller_bcard_t::begin_session_mailbox_t begin_session_mailbox;
        backfiller_bcard_t::end_session_mailbox_t end_session_mailbox;
        backfiller_bcard_t::ack_items_mailbox_t ack_items_mailbox;
    };

    mailbox_manager_t *const mailbox_manager;
    branch_history_manager_t *const branch_history_manager;
    store_view_t *const store;

    registrar_t<backfiller_bcard_t::intro_1_t, backfiller_t *, client_t> registrar;

    DISABLE_COPYING(backfiller_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_ */
