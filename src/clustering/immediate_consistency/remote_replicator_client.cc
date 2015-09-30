// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_client.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/backfillee.hpp"
#include "clustering/table_manager/backfill_progress_tracker.hpp"
#include "stl_utils.hpp"
#include "store_view.hpp"

class remote_replicator_client_t::timestamp_range_tracker_t {
public:
    timestamp_range_tracker_t(
            const region_t &_store_region,
            state_timestamp_t _prev_timestamp) :
        store_region(_store_region),
        prev_timestamp(_prev_timestamp) { }

    /* Records that the backfill has copied values up to the given timestamp for the
    given range. */
    void record_backfill(const region_t &region, const state_timestamp_t &ts) {
        rassert(region.beg == store_region.beg);
        rassert(region.end == store_region.end);
        rassert(key_range_t::right_bound_t(region.inner.left) ==
            (entries.empty()
                ? key_range_t::right_bound_t(store_region.inner.left)
                : entries.back().first));
        rassert(region.inner.right <= store_region.inner.right);
        if (entries.empty() || entries.back().second != ts) {
            entries.push_back(std::make_pair(region.inner.right, ts));
        } else {
            /* Rather than making a second entry with the same timestamp, coalesce
            the two entries. */
            entries.back().first = region.inner.right;
        }
    }

    /* Records that the streaming write with the given timestamp has been applied in the
    given region. */
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
        rassert(entries[0].second.next() == ts);
        entries[0].second = ts;
        if (entries.size() > 1 && entries[1].second == ts) {
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
            return key_range_t::right_bound_t(store_region.inner.left);
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
    to clip the write, so this will crash. */
    void clip_next_write_backfilling(
            state_timestamp_t timestamp, region_t *region_out) {
        guarantee(can_clip_next_write_backfilling());
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

    /* The timestamp of the last streaming write that has been applied. */
    state_timestamp_t prev_timestamp;

    /* Each entry in `entries` represents a possibly-empty range. The right bound of
    the range is the `right_bound_t` on the entry; the left bound of the range is the
    `right_bound_t` on the previous entry, or `store_region.inner.left` for the very
    first entry.

    The entries' `right_bound_t`s and timestamps will be strictly monotonically
    increasing, but the difference between adjacent entries' timestamps may be more
    than one. */
    std::deque<std::pair<key_range_t::right_bound_t, state_timestamp_t> > entries;
};

remote_replicator_client_t::remote_replicator_client_t(
        backfill_throttler_t *backfill_throttler,
        const backfill_config_t &backfill_config,
        backfill_progress_tracker_t *backfill_progress_tracker,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,
        backfill_throttler_t::priority_t::critical_t is_critical_priority,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,
        const server_id_t &primary_server_id,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) :

    mailbox_manager_(mailbox_manager),
    store_(store),
    region_(store->get_region()),
    branch_id_(branch_id),
    mode_(backfill_mode_t::PAUSED),

    next_write_waiter_(nullptr),

    write_async_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_write_async, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5)),
    write_sync_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_write_sync, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6)),
    dummy_write_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_dummy_write, this,
            ph::_1, ph::_2)),
    read_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_read, this,
            ph::_1, ph::_2, ph::_3, ph::_4))
{
    guarantee(remote_replicator_server_bcard.branch == branch_id);
    guarantee(remote_replicator_server_bcard.region == region_);

    auto progress_tracker =
        backfill_progress_tracker->insert_progress_tracker(region_);
    progress_tracker->is_ready = false;
    progress_tracker->start_time = current_microtime();
    progress_tracker->source_server_id = primary_server_id;
    progress_tracker->progress = 0.0;

    /* If the store is currently constructing a secondary index, wait until it finishes
    before we start the backfill. We'll also check again periodically during the
    backfill. */
    store->wait_until_ok_to_receive_backfill(interruptor);

    /* Subscribe to the stream of writes coming from the primary */
    remote_replicator_client_intro_t intro;
    {
        cond_t got_intro;
        remote_replicator_client_bcard_t::intro_mailbox_t intro_mailbox(
            mailbox_manager,
            [&](signal_t *, const remote_replicator_client_intro_t &i) {
                intro = i;
                mode_ = backfill_mode_t::PAUSED;
                timestamp_enforcer_.init(new timestamp_enforcer_t(
                    intro.streaming_begin_timestamp));
                tracker_.init(new timestamp_range_tracker_t(
                    region_, intro.streaming_begin_timestamp));
                got_intro.pulse();
            });
        remote_replicator_client_bcard_t our_bcard {
            server_id,
            intro_mailbox.get_address(),
            write_async_mailbox_.get_address(),
            write_sync_mailbox_.get_address(),
            dummy_write_mailbox_.get_address(),
            read_mailbox_.get_address() };
        registrant_.init(new registrant_t<remote_replicator_client_bcard_t>(
            mailbox_manager, remote_replicator_server_bcard.registrar, our_bcard));
        wait_interruptible(&got_intro, interruptor);
        registered_.pulse();
    }

    /* OK, now we're streaming writes from the primary, but they're being discarded as
    they arrive because `tracker_` indicates that nothing has been backfilled. */

    backfillee_t backfillee(mailbox_manager, branch_history_manager, store,
        replica_bcard.backfiller_bcard, backfill_config, progress_tracker, interruptor);

    while (tracker_->get_backfill_threshold() != region_.inner.right) {

        /* If the store is currently constructing a secondary index, wait until it
        finishes before we do the next phase of the backfill. This is the correct phase
        of the backfill cycle at which to wait because we aren't currently receiving
        anything from the backfiller and we aren't piling up changes in any queues. */
        store->wait_until_ok_to_receive_backfill(interruptor);

        /* Acquire the backfill throttler lock. */
        backfill_throttler_t::priority_t priority;
        priority.critical = is_critical_priority;
        priority.num_changes = backfillee.get_num_changes_estimate();
        backfill_throttler_t::lock_t backfill_throttler_lock(
            backfill_throttler, priority, interruptor);

        state_timestamp_t backfill_start_timestamp;
        {
            mutex_assertion_t::acq_t mutex_assertion_acq(&mutex_assertion_);
            guarantee(mode_ == backfill_mode_t::PAUSED);
            mode_ = backfill_mode_t::BACKFILLING;
            backfill_start_timestamp =
                timestamp_enforcer_->get_latest_all_before_completed();
            rassert(backfill_start_timestamp == tracker_->get_prev_timestamp());
        }

        /* Block until backfiller reaches `backfill_start_timestamp`, to ensure that the
        backfill end timestamp will be at least `backfill_start_timestamp`. In the first
        time through the loop, this is important to ensure that there isn't any gap
        between the backfilled data and the streamed writes; in subsequent iterations,
        this is important to ensure that if we threw away part of a streamed change while
        in `PAUSED` mode, we'll definitely get that part of that write as part of the
        backfill. */
        {
            cond_t backfiller_is_up_to_date;
            mailbox_t<void()> ack_mbox(
                mailbox_manager,
                [&](signal_t *) { backfiller_is_up_to_date.pulse(); });
            send(mailbox_manager, replica_bcard.synchronize_mailbox,
                backfill_start_timestamp, ack_mbox.get_address());
            wait_interruptible(&backfiller_is_up_to_date, interruptor);
        }

        /* Backfill in lexicographical order until we finish or the backfill throttler
        lock tells us to pause again */
        class callback_t : public backfillee_t::callback_t {
        public:
            callback_t(remote_replicator_client_t *p, signal_t *ps) :
                parent(p), preempt_signal(ps) { }
            bool on_progress(const region_map_t<version_t> &chunk) THROWS_NOTHING {
                mutex_assertion_t::acq_t mutex_assertion_acq(&parent->mutex_assertion_);
                chunk.visit(chunk.get_domain(),
                [&](const region_t &reg, const version_t &vers) {
                    parent->tracker_->record_backfill(reg, vers.timestamp);
                });
                if (parent->next_write_can_proceed(&mutex_assertion_acq)) {
                    if (parent->next_write_waiter_ != nullptr) {
                        parent->next_write_waiter_->pulse_if_not_already_pulsed();
                    }
                }
                /* If the backfill throttler is telling us to pause, then interrupt
                `backfillee.go()` */
                return !preempt_signal->is_pulsed();
            }
            remote_replicator_client_t *parent;
            signal_t *preempt_signal;
        } callback(this, backfill_throttler_lock.get_preempt_signal());

        backfillee.go(
            &callback,
            tracker_->get_backfill_threshold(),
            interruptor);

        if (tracker_->get_backfill_threshold() != region_.inner.right) {
            guarantee(backfill_throttler_lock.get_preempt_signal()->is_pulsed());
            /* Switch mode to `PAUSED` so that writes can proceed while we wait to
            reacquire the throttler lock */
            mutex_assertion_t::acq_t mutex_assertion_acq(&mutex_assertion_);
            guarantee(mode_ == backfill_mode_t::BACKFILLING);
            mode_ = backfill_mode_t::PAUSED;
            if (next_write_waiter_ != nullptr) {
                /* Writes can always proceed immediately in `PAUSED` mode */
                next_write_waiter_->pulse_if_not_already_pulsed();
            }
        }
    }

    /* Wait until writes execute up to the point where the backfill left us, so that
    `tracker_->is_homogeneous()` will be `true`. */
    timestamp_enforcer_->wait_all_before(tracker_->get_max_timestamp(), interruptor);

    {
        /* Lock out writes again because some of these final operations might block */
        rwlock_acq_t cleanup_rwlock_acq(&cleanup_rwlock_, access_t::write, interruptor);
        mutex_assertion_t::acq_t mutex_assertion_acq(&mutex_assertion_);

        guarantee(tracker_->is_homogeneous());
        guarantee(tracker_->get_prev_timestamp() ==
            timestamp_enforcer_->get_latest_all_before_completed());

#ifndef NDEBUG
        /* Sanity check that the store's metainfo is all on the correct branch and
        all at the correct timestamp */
        read_token_t read_token;
        store->new_read_token(&read_token);
        region_map_t<version_t> version = to_version_map(store->get_metainfo(
            order_token_t::ignore.with_read_mode(), &read_token, region_,
            interruptor));
        version_t expect(branch_id,
            timestamp_enforcer_->get_latest_all_before_completed());
        version.visit(region_,
        [&](const region_t &region, const version_t &actual) {
            rassert(actual == expect, "Expected version %s for sub-range %s, but "
                "got version %s.", debug_strprint(expect).c_str(),
                debug_strprint(region).c_str(), debug_strprint(actual).c_str());
        });
#endif

        /* Now we're completely up-to-date and synchronized with the primary, it's time
        to create a `replica_t`. */
        replica_.init(new replica_t(mailbox_manager_, store_, branch_history_manager,
            branch_id, timestamp_enforcer_->get_latest_all_before_completed()));

        tracker_.reset();   /* we don't need `tracker_` anymore */
        mode_ = backfill_mode_t::STREAMING;

        if (next_write_waiter_ != nullptr) {
            /* Writes can always proceed immediately in `STREAMING` mode */
            next_write_waiter_->pulse_if_not_already_pulsed();
        }
    }

    /* Now that we're completely up-to-date, tell the primary that it's OK to send us
    reads and synchronous writes */
    send(mailbox_manager, intro.ready_mailbox);
}

remote_replicator_client_t::~remote_replicator_client_t() {
    /* The destructor is declared here instead of the header file so that we can see the
    destructor for `timestamp_range_tracker_t` */
}

void remote_replicator_client_t::on_write_async(
        signal_t *interruptor,
        write_t &&write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        const mailbox_t<void()>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    wait_interruptible(&registered_, interruptor);

    timestamp_enforcer_->wait_all_before(timestamp.pred(), interruptor);

    rwlock_acq_t cleanup_rwlock_acq(&cleanup_rwlock_, access_t::read, interruptor);

    mutex_assertion_t::acq_t mutex_assertion_acq(&mutex_assertion_);
    while (!next_write_can_proceed(&mutex_assertion_acq)) {
        cond_t cond;
        guarantee(next_write_waiter_ == nullptr);
        assignment_sentry_t<cond_t *> cond_sentry(&next_write_waiter_, &cond);
        mutex_assertion_acq.reset();
        wait_interruptible(&cond, interruptor);
        mutex_assertion_acq.reset(&mutex_assertion_);
    }

    if (mode_ == backfill_mode_t::STREAMING) {
        /* Once the constructor is done, all writes will take this branch; it's the
        common case. */

        timestamp_enforcer_->complete(timestamp);

        /* Release the locks before we start the slow part */
        mutex_assertion_acq.reset();
        cleanup_rwlock_acq.reset();

        write_response_t dummy_response;
        replica_->do_write(write, timestamp, order_token, write_durability_t::SOFT,
            interruptor, &dummy_response);
    } else {
        region_t clip_region;
        if (mode_ == backfill_mode_t::PAUSED) {
            tracker_->clip_next_write_paused(timestamp, &clip_region);
        } else {
            tracker_->clip_next_write_backfilling(timestamp, &clip_region);
        }
        tracker_->record_write(clip_region, timestamp);
        write_token_t token;
        store_->new_write_token(&token);
        timestamp_enforcer_->complete(timestamp);

        /* Release the locks before we start the slow part */
        mutex_assertion_acq.reset();
        cleanup_rwlock_acq.reset();

        if (!region_is_empty(clip_region)) {
            region_map_t<binary_blob_t> new_metainfo(
                clip_region, binary_blob_t(version_t(branch_id_, timestamp)));
            write_t subwrite;
            if (write.shard(clip_region, &subwrite)) {
#ifndef NDEBUG
                metainfo_checker_t checker(clip_region,
                    [&](const region_t &, const binary_blob_t &bb) {
                        rassert(bb == binary_blob_t(
                            version_t(branch_id_, timestamp.pred())));
                    });
#endif
                write_response_t dummy_response;
                store_->write(DEBUG_ONLY(checker, ) new_metainfo, subwrite,
                    &dummy_response, write_durability_t::SOFT, timestamp, order_token,
                    &token, interruptor);
            } else {
                /* The write doesn't actually affect any keys in this region, but we
                still have to update the metainfo for consistency's sake. */
                store_->set_metainfo(new_metainfo, order_token, &token,
                    write_durability_t::SOFT, interruptor);
            }
        }
    }

    send(mailbox_manager_, ack_addr);
}

void remote_replicator_client_t::on_write_sync(
        signal_t *interruptor,
        const write_t &write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        write_durability_t durability,
        const mailbox_t<void(write_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    /* The current implementation of the dispatcher will never send us an async write
    once it's started sending sync writes, but we don't want to rely on that detail, so
    we pass sync writes through the timestamp enforcer too. */
    timestamp_enforcer_->complete(timestamp);

    write_response_t response;
    replica_->do_write(
        write, timestamp, order_token, durability,
        interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

void remote_replicator_client_t::on_dummy_write(
        signal_t *interruptor,
        const mailbox_t<void(write_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    write_response_t response;
    replica_->do_dummy_write(interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

void remote_replicator_client_t::on_read(
        signal_t *interruptor,
        const read_t &read,
        state_timestamp_t min_timestamp,
        const mailbox_t<void(read_response_t)>::address_t &ack_addr)
        THROWS_ONLY(interrupted_exc_t) {
    read_response_t response;
    replica_->do_read(read, min_timestamp, interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

bool remote_replicator_client_t::next_write_can_proceed(
        mutex_assertion_t::acq_t *mutex_assertion_acq) {
    mutex_assertion_acq->assert_is_holding(&mutex_assertion_);
    return mode_ != backfill_mode_t::BACKFILLING ||
        tracker_->can_clip_next_write_backfilling();
}

