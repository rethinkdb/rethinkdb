// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_CLIENT_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_CLIENT_HPP_

#include <queue>

#include "clustering/generic/registrant.hpp"
#include "clustering/immediate_consistency/remote_replicator_metadata.hpp"
#include "clustering/immediate_consistency/replica.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "concurrency/semaphore.hpp"

class backfill_progress_tracker_t;
class backfill_throttler_t;

/* `remote_replicator_client_t` contacts a `remote_replicator_server_t` on another server
to sign up for writes to a given shard, and then applies them to a `store_t` on the same
server. It also performs a backfill to initialize the state of the `store_t`, and handles
the transition between the backfill and the streaming writes.

There is one `remote_replicator_client_t` on each secondary replica server of each shard.
`secondary_execution_t` constructs it. */

class remote_replicator_client_t {
public:
    remote_replicator_client_t(
        backfill_throttler_t *backfill_throttler,
        const backfill_config_t &backfill_config,
        backfill_progress_tracker_t *backfill_progress_tracker,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,
        const server_id_t &primary_server_id,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

private:
    /* `on_write_async()`, `on_write_sync()`, and `on_read()` are mailbox callbacks for
    `write_async_mailbox_`, `write_sync_mailbox_`, and `read_mailbox_`. */
    void on_write_async(
            signal_t *interruptor,
            write_t &&write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            const mailbox_t<void()>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t);

    void on_write_sync(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_durability_t durability,
            const mailbox_t<void(write_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t);

    void on_read(
            signal_t *interruptor,
            const read_t &read,
            state_timestamp_t min_timestamp,
            const mailbox_t<void(read_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t);

    mailbox_manager_t *const mailbox_manager_;
    store_view_t *const store_;
    region_t const region_;   /* same as `store_->get_region()` */
    branch_id_t const branch_id_;

    /* During the constructor, we alternate between `PAUSED` and `BACKFILLING`. When the
    constructor is done, we switch to `STREAMING` mode and stay there. */
    enum class backfill_mode_t {
        /* We haven't backfilled completely, but we aren't currently backfilling either.
        Usually this means we're waiting for the backfill throttler. However, we will
        still accept streaming writes in the regions we've already backfilled. */
        PAUSED,
        /* We're actively receiving backfill items over the network. Whenever the
        backfill reaches a given timestamp, we'll also apply all writes at or before that
        timestamp; this is mediated by `backfilling_thresholds_`. */
        BACKFILLING,
        /* We finished backfilling and we're just applying writes as they arrive. */
        STREAMING
        };
    backfill_mode_t mode_;

    /* `timestamp_range_tracker_t` is essentially a `region_map_t<state_timestamp_t>`,
    but in a different format. The domain is the region that has been backfilled thus
    far; the values are equal to the current timestamps in the B-tree metainfo. */
    class timestamp_range_tracker_t {
    public:
        timestamp_range_tracker_t(
                const region_t &_store_region,
                state_timestamp_t _prev_timestamp) :
            store_region(_store_region),
            prev_timestamp(_prev_timestamp) { }

        /* Records that the backfill has copied values up to the given timestamp for the
        given range. */
        void record_backfill(const region_t &region, const timestamp_t &ts) {
            rassert(region.beg == store_region.beg);
            rassert(region.end == store_region.end);
            rassert(key_range_t::right_bound_t(region.left) ==
                (entries.empty()
                    ? key_range_t::right_bound_t(store_region.left)
                    : entries.back().first));
            rassert(region.right <= store_region.right);
            if (entries.empty() || entries.back().second != ts) {
                entries.push_back(std::make_pair(region.right, ts));
            } else {
                /* Rather than making a second entry with the same timestamp, coalesce
                the two entries. */
                entries.back().first = region.right;
            }
        }

        /* Records that the write with the given timestamp has been applies in the given
        region. */
        void record_write(const region_t &region, state_timestamp_t ts) {
            rassert(ts == prev_timestamp.next());
            prev_timestamp = ts;
            if (region_is_empty(region)) {
                rassert(entries.empty() || entries[0].second >= ts);
                return;
            }
            rassert(region.beg == store_region.beg);
            rassert(region.end == store_region.end);
            rassert(region.inner.left == store_region.inner.left);
            rassert(region.inner.right == entries[0].first);
            entries[0].second = timestamp;
            if (entries.size() > 1 && entries[1].second == timestamp) {
                entries.pop_front();
            }
        }

        /* Returns the timestamp of the last streaming write processed. */
        state_timestamp_t get_prev_timestamp() const {
            return prev_timestamp;
        }

        /* Returns the highest timestamp present in the map */
        state_timestamp_t get_max_timestamp() const {
            if (entries.empty()) {
                return prev_timestamp;
            } else {
                return entries.back().second;
            }
        }

        /* Returns the rightmost point we've backfilled to so far */
        key_range_t::right_bound_t get_backfill_threshold() const {
            if (entries.empty()) {
                return key_range_t::right_bound_t(store_region.left);
            } else {
                return entries.back().first;
            }
        }

        /* Returns `true` if the timestamp is consistent throughout the entire region */
        bool is_homogeneous() const {
            return !entries.empty() && entries[0].first == store_region.inner.right;
        }

        /* Given the next streaming write, extracts the part of it that should be applied
        such that the streaming write and the backfill together will neither skip nor
        duplicate any change. If the backfill hasn't sent us any changes with timestamps
        equal to or greater than the next write, we wouldn't be able to determine where
        to clip the write, so this will crash. It also updates the
        `timestamp_range_tracker_t`'s internal record to reflect the fact that the write
        will be applied to the store. */
        void clip_next_write_backfilling(
                state_timestamp_t timestamp, region_t *region_out) {
            guarantee(can_process_next_write_backfilling());
            clip_next_write_paused(timestamp, region_out);
        }

        /* Returns `true` if the backfill has sent us some changes with timestamps equal
        to or greater than the next write. */
        bool can_clip_next_write_backfilling() const {
            return !entries.empty() &&
                (entries.size() > 1 || entries[0].first == store_region.inner.right);
        }

        /* Similar to `clip_next_write_backfilling()`, except that if the backfill
        hasn't sent us any changes with timestamps equal to or greater than the next
        write, it will assume that the timestamps in all yet-to-be-backfilled regions
        will be equal to or greater than the next write. Therefore, this is only safe to
        use in `PAUSED` mode; when we exit `PAUSED` mode, we'll ensure that the backfill
        resumes from a timestamp equal to or greater than the next write. */
        void clip_next_write_paused(
                state_timestamp_t timestamp, region_t *region_out) {
            guarantee(timestamp == prev_timestamp.next(), "sanity check failed");
            if (entries.empty() || entries[0].second > prev_timestamp) {
                *region_out = region_t::empty();
                return;
            }
            guarantee(entries[0].second == prev_timestamp);
            *region_out = store_region;
            region_out->inner.right = entries.front().first;
        }

    private:
        /* Same as `remote_replicator_client_t::store_->get_region()`. */
        region_t store_region;

        /* The timestamp of the next streaming write that should be applied. */
        state_timestamp_t prev_timestamp;

        /* Each entry in `entries` represents a possibly-empty range. The right bound of
        the range is the `right_bound_t` on the entry; the left bound of the range is the
        `right_bound_t` on the previous entry, or `store_region.inner.left` for the very
        first entry. There will always be at least one entry.

        The entries' `right_bound_t`s and timestamps will be strictly monotonically
        increasing, but the difference between adjacent entries' timestamps may be more
        than one. */
        std::deque<std::pair<key_range_t::right_bound_t, state_timestamp_t> > entries;

    };
    scoped_ptr_t<timestamp_range_tracker_t> tracker_;

    /* Returns `true` if the next write can be applied now, instead of having to wait for
    the backfill to make more progress. */
    bool next_write_can_proceed(const mutex_assertion_t::acq_t *mutex_acq);

    /* If the next write cannot proceed, it will set `next_write_waiter_` and wait for it
    to be pulsed. */
    cond_t *next_write_waiter_;

    /* `timestamp_enforcer_` is used to order writes as they arrive. `replica_` will do
    its own ordering of writes, so `timestamp_enforcer_` is only important during the
    constructor before `replica_` has been created. */
    scoped_ptr_t<timestamp_enforcer_t> timestamp_enforcer_;

    /* `replica_` is created at the end of the constructor, once the backfill is over. */
    scoped_ptr_t<replica_t> replica_;

    mutex_assertion_t mutex_assertion_;

    rwlock_t rwlock_;

    remote_replicator_client_bcard_t::write_async_mailbox_t write_async_mailbox_;
    remote_replicator_client_bcard_t::write_sync_mailbox_t write_sync_mailbox_;
    remote_replicator_client_bcard_t::read_mailbox_t read_mailbox_;

    /* We use `registrant_` to subscribe to a stream of reads and writes from the
    dispatcher via the `remote_replicator_server_t`. */
    scoped_ptr_t<registrant_t<remote_replicator_client_bcard_t> > registrant_;
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_REMOTE_REPLICATOR_HPP_ */

