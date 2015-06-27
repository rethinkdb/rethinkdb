// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_client.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/backfillee.hpp"
#include "clustering/table_manager/backfill_progress_tracker.hpp"
#include "stl_utils.hpp"
#include "store_view.hpp"

remote_replicator_client_t::remote_replicator_client_t(
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

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) :

    mailbox_manager_(mailbox_manager),
    store_(store),
    region_(store->get_region()),
    branch_id_(branch_id),

    write_async_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_write_async, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5)),
    write_sync_mailbox_(mailbox_manager,
        std::bind(&remote_replicator_client_t::on_write_sync, this,
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6)),
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
        remote_replicator_client_bcard_t::intro_mailbox_t intro_mailbox(
            mailbox_manager,
            [&](signal_t *, const remote_replicator_client_intro_t &i) {
                intro = i;
                mode_ = backfill_mode_t::PAUSED;
                timestamp_enforcer_.init(new timestamp_enforcer_t(
                    intro.streaming_begin_timestamp));
                tracker_.init(new timestamp_range_tracker_t(
                    regeion_, intro.streaming_begin_timestamp));
                registered_.pulse();
            });
        remote_replicator_client_bcard_t our_bcard {
            server_id,
            intro_mailbox.get_address(),
            write_async_mailbox_.get_address(),
            write_sync_mailbox_.get_address(),
            read_mailbox_.get_address() };
        registrant_.init(new registrant_t<remote_replicator_client_bcard_t>(
            mailbox_manager, remote_replicator_server_bcard.registrar, our_bcard));
        wait_interruptible(&registered_, interruptor);
    }

    /* OK, now we're streaming writes from the primary, but they're being discarded as
    they arrive because `thresholds_` indicates that nothing has been backfilled. */

    backfillee_t backfillee(mailbox_manager, branch_history_manager, store,
        replica_bcard.backfiller_bcard, backfill_config, progress_tracker, interruptor);

    /* We acquire `rwlock_` to lock out writes while we're writing to `region_*_`,
    `queue_fun_`, and `replica_`, and for the last stage of draining the queue. */
    scoped_ptr_t<rwlock_acq_t> rwlock_acq(
        new rwlock_acq_t(&rwlock_, access_t::write, interruptor));

    while (tracker_->get_backfill_threshold() != region_.inner.right) {
        rwlock_acq.reset();

        /* If the store is currently constructing a secondary index, wait until it
        finishes before we do the next phase of the backfill. This is the correct phase
        of the backfill cycle at which to wait because we aren't currently receiving
        anything from the backfiller and we aren't piling up changes in any queues. */
        store->wait_until_ok_to_receive_backfill(interruptor);

        backfill_throttler_t::lock_t backfill_throttler_lock(
            backfill_throttler,
            replica_bcard.synchronize_mailbox.get_peer(),
            interruptor);

        rwlock_acq.init(new rwlock_acq_t(&rwlock_, access_t::write, interruptor));

        guarantee(mode_ == backfill_mode_t::PAUSED);
        mode_ = backfill_mode_t::BACKFILLING;
        state_timestamp_t backfill_start_timestamp =
            timestamp_enforcer_->get_latest_all_before_completed();
        rassert(backfill_start_timestamp == tracker_->get_prev_timestamp());

        rwlock_acq.reset();

        /* Block until backfiller reaches `backfill_start_timestamp`, to ensure that the
        backfill end timestamp will be at least `backfill_start_timestamp` */
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
                parent(p), pause_signal(ps) { }
            bool on_progress(const region_map_t<version_t> &chunk) THROWS_NOTHING {
                auto *t = &parent->thresholds_;
                rassert(pulse_on_progress_ == nullptr || pulse_on_progress_->is_pulsed()
                    || !tracker_->can_process_next_write_backfilling(),
                    "pulse_on_progress_ shouldn't be necessary because on_write "
                    "should already be able to proceed");
                chunk.visit(chunk.get_domain(),
                [&](const region_t &reg, const version_t &vers) {
                    tracker_->record_backfill(reg, vers.timestamp);
                });
                if (parent->pulse_on_progress_ != nullptr &&
                        tracker_->can_process_next_write_backfilling()) {
                    parent->pulse_on_progress_->pulse_if_not_already_pulsed();
                }
                /* If the backfill throttler is telling us to pause, then interrupt
                `backfillee.go()` */
                return !pause_signal->is_pulsed();
            }
            remote_replicator_client_t *parent;
            signal_t *pause_signal;
        } callback(this, backfill_throttler_lock.get_pause_signal());

        backfillee.go(
            &callback,
            tracker_->get_backfill_threshold(),
            interruptor);

        rwlock_acq.init(new rwlock_acq_t(&rwlock_, access_t::write, interruptor));

        guarantee(mode_ == backfill_mode_t::BACKFILLING);
        mode_ = backfill_mode_t::PAUSED;
    }

#ifndef NDEBUG
    {
        /* Sanity check that the store's metainfo is all on the correct branch and all at
        the correct timestamp */
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
    }
#endif

    /* Now we're completely up-to-date and synchronized with the primary, it's time to
    create a `replica_t`. */
    replica_.init(new replica_t(mailbox_manager_, store_, branch_history_manager,
        branch_id, timestamp_enforcer_->get_latest_all_before_completed()));

    mode_ = backfill_mode_t::STREAMING

    rwlock_acq.reset();

    /* Now that we're completely up-to-date, tell the primary that it's OK to send us
    reads and synchronous writes */
    send(mailbox_manager, intro.ready_mailbox);
}

void remote_replicator_client_t::apply_write_or_metainfo(
        store_view_t *store,
        const branch_id_t &branch_id,
        const region_t &region,
        bool has_write,
        const write_t &write,
        state_timestamp_t timestamp,
        write_token_t *token,
        order_token_t order_token,
        signal_t *interruptor) {
    region_map_t<binary_blob_t> new_metainfo(
        region, binary_blob_t(version_t(branch_id, timestamp)));
    if (has_write) {
#ifndef NDEBUG
        metainfo_checker_t checker(region,
            [&](const region_t &, const binary_blob_t &bb) {
                rassert(bb == binary_blob_t(version_t(branch_id, timestamp.pred())));
            });
#endif
        write_response_t dummy_response;
        store->write(
            DEBUG_ONLY(checker, )
            new_metainfo,
            write,
            &dummy_response,
            write_durability_t::SOFT,
            timestamp,
            order_token,
            token,
            interruptor);
    } else {
        store->set_metainfo(
            new_metainfo,
            order_token,
            token,
            write_durability_t::SOFT,
            interruptor);
    }
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

    rwlock_acq_t rwlock_acq(&rwlock_, access_t::read, interruptor);

    switch (mode_) {
    case backfill_mode_t::STREAMING: {
        /* Once the constructor is done, all writes will take this branch; it's the
        common case. */
        timestamp_enforcer_->complete(timestamp);
        rwlock_acq.reset();

        write_response_t dummy_response;
        replica_->do_write(write, timestamp, order_token, write_durability_t::SOFT,
            interruptor, &dummy_response);
        break;
    }
    case backfill_mode_t::BACKFILLING: {
        /* First, we block until we backfill some data with a timestamp higher than this
        write, or we backfill all the way to the right bound. */
        rassert(timestamp == tracker_->get_prev_timestamp().next());
        if (mode_ == backfill_mode_t::BACKFILLING &&
                !tracker_->can_process_next_writee_backfilling()) {
            rassert(pulse_on_progress_ == nullptr);
            cond_t cond;
            assignment_sentry_t<cond_t *> cond_sentry(&pulse_on_progress_, &cond);
            wait_interruptible(&cond, interruptor);
            rassert(!(thresholds.size() == 1 &&
                thresholds_.front().first != region_.inner.right));
        }

        /* Make a local copy of `region_streaming_` because it might change once we
        release `rwlock_acq`. */
        region_t region_streaming_copy = region_streaming_;
        write_t subwrite_streaming;
        bool have_subwrite_streaming = false;
        write_token_t write_token_streaming;
        if (!region_is_empty(region_streaming_copy)) {
            have_subwrite_streaming =
                write.shard(region_streaming_, &subwrite_streaming);
            store_->new_write_token(&write_token_streaming);
        }

        cond_t queue_throttler;
        if (queue_fun_ != nullptr) {
            rassert(!region_is_empty(region_queueing_));
            queue_entry_t queue_entry;
            queue_entry.has_write = write.shard(region_queueing_, &queue_entry.write);
            queue_entry.timestamp = timestamp;
            queue_entry.order_token = queue_order_checkpoint_.check_through(order_token);
            (*queue_fun_)(std::move(queue_entry), &queue_throttler);
        } else {
            /* Usually the only reason for `queue_fun_` to be null would be if we're
            currently between two queueing phases. But it could also be null if the
            constructor just got interrupted. */
            queue_throttler.pulse();
        }

        timestamp_enforcer_->complete(timestamp);
        rwlock_acq.reset();

        if (!region_is_empty(region_streaming_copy)) {
            apply_write_or_metainfo(store_, branch_id_, region_streaming_copy,
                have_subwrite_streaming, subwrite_streaming, timestamp,
                &write_token_streaming, order_token, interruptor);
        }

        /* Wait until the queueing logic pulses our `queue_throttler`. The dispatcher
        will limit the number of outstanding writes to us at any given time; so if we
        delay acking this write, that will limit the rate at which the dispatcher sends
        us new writes. The constructor uses this to ensure that new writes enter the
        queue more slowly than writes are being removed from the queue. */
        wait_interruptible(&queue_throttler, interruptor);
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

