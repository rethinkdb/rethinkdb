// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_client.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/backfillee.hpp"
#include "store_view.hpp"

class remote_replicator_client_t::backfill_end_timestamps_t {
public:
    backfill_end_timestamps_t() : region(region_t::empty()) { }
    backfill_end_timestamps_t(const region_map_t<state_timestamp_t> &region_map) {
        region = region_map->get_domain();
        std::vector<std::pair<store_key_t, state_timestamp_t> > temp;
        for (const auto &pair : region_map) {
            temp.push_back(std::make_pair(pair.first.inner.left, pair.second));
        }
        std::sort(temp.begin(), temp.end());
        for (const auto &step : temp) {
            if (!steps.empty()) {
                guarantee(step.second >= steps.back().second);
                if (step.second == steps.back().second) {
                    continue;
                }
            }
            steps.push_back(step);
        }
        max_timestamp = steps.back().second;
    }
    state_timestamp_t get_max_timestamp() const {
        return max_timestamp;
    }
    bool clip_write(write_t *w, state_timestamp_t ts) const {
        rassert(region_is_superset(region, w->get_region()));
        if (ts > max_timestamp) {
            /* Shortcut for the simple case */
            return;
        }
        region_t clip_region = w->get_region();
        for (const auto &step : steps) {
            if (step.second >= ts) {
                clip_region.right = key_range_t::right_bound_t(step.first);
                break;
            }
        }
        write_t clipped;
        if (!w->shard(clip_region, &clipped)) {
            return false;
        }
        *w = std::move(clipped);
        return true;
    }
    void combine(backfill_end_timestamps_t &&next) {
        if (region.is_empty()) {
            *this = std::move(next);
        } else {
            guarantee(region.beg == next.region.beg && region.end == next.region.end);
            guarantee(region.inner.right ==
                key_range_t::right_bound_t(next.region.inner.left));
            region.inner.right = next.region.inner.right;
            steps.insert(
                steps.end(),
                std::make_move_iterator(next.steps.begin()),
                std::make_move_iterator(next.steps.end()));
            max_timestamp = std::max(max_timestamp, next.max_timestamp);
        }
    }
private:
    region_t region;
    state_timestamp_t max_timestamp;
    std::vector<std::pair<store_key_t, state_timestamp_t> > steps;
};

remote_replicator_client_t::remote_replicator_client_t(
        backfill_throttler_t *backfill_throttler,
        mailbox_manager_t *mailbox_manager,
        const server_id_t &server_id,

        const branch_id_t &branch_id,
        const remote_replicator_server_bcard_t &remote_replicator_server_bcard,
        const replica_bcard_t &replica_bcard,

        store_view_t *store,
        branch_history_manager_t *branch_history_manager,

        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) :

    mailbox_manager_(mailbox_manager),
    store_(store),
    branch_id_(branch_id),
    queue_(nullptr),
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
    /* Initially, the streaming and queueing regions are empty, and the discarding region
    is the entire key-space. */
    region_streaming_ = region_queueing_ = region_discarding_ = store->get_region();
    region_streaming_.inner.right = region_queueing_.inner.right = key_range_t::empty();

    /* Subscribe to the stream of writes coming from the primary */
    guarantee(remote_replicator_server_bcard.branch == branch_id);
    remote_replicator_client_intro_t intro;
    {
        remote_replicator_client_bcard_t::intro_mailbox_t intro_mailbox(
            mailbox_manager,
            [&](signal_t *, const remote_replicator_client_intro_t &i) {
                intro = i;
                timestamp_enforcer.init(new timestamp_enforcer_t(
                    intro.streaming_begin_timestamp));
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
    they arrive because `discard_threshold_` is the left boundary. */

    backfillee_t backfillee(mailbox_manager, branch_history_manager, store,
        replica_bcard.backfiller, interruptor);

    scoped_ptr_t<rwlock_acq_t> rwlock_acq(
        new rwlock_acq_t(&lock_regions, access_t::write, interruptor));
    while (region_streaming_.inner.right != store->get_region().inner.right) {
        /* Previously we were streaming some sub-range and discarding the rest. Here we
        leave the streaming region as it was but we start queueing the region we were
        previously discarding. */
        region_queueing_ = region_discarding_;
        region_discarding_.inner = key_range_t::empty();
        std::queue<queue_entry_t> queue;
        queue_function_t queue_fun;
        queue_fun = [&](queue_entry_t &&entry, cond_t *ack) {
            queue.push(std::move(entry));
            ack->pulse();
        };
        assignment_sentry_t<queue_function_t *> queue_sentry(&queue_fun_, &queue_fun);
        state_timestamp_t backfill_start_timestamp =
            timestamp_enforcer_->get_latest_all_before_completed();

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

        /* Backfill in lexicographical order until the queue hits a certain size */
        class callback_t : public backfillee_t::callback_t {
        public:
            store_callback_t::backfill_continue_t on_progress(
                    const region_map_t<version_t> &chunk) {
                backfill_end_timestamps.combine(
                    region_map_transform<version_t, state_timestamp_t>(chunk,
                        [](const version_t &version) { return version.timestamp; }));
                if (parent->queue_ptr_->size() > 1000) {
                    return store_callback_t::backfill_continue_t::STOP_AFTER;
                } else {
                    return store_callback_t::backfill_continue_t::CONTINUE;
                }
            }
            remote_replicator_client_t *parent;
            backfill_end_timestamps_t backfill_end_timestamps;
            key_range_t::right_bound_t right_bound;
        } callback { this, key_range_t::right_bound_t(region_queueing_.inner.left) };
        backfillee.go(&callback, interruptor);

        rwlock_acq.init(new rwlock_acq_t(&lock_regions, access_t::write, interruptor));

        /* Shrink the queueing region to only contain the region that we just backfilled,
        so we discard any changes to the right of that */
        region_queueing_.inner.right = callback.right_bound;
        if (region_queueing_.inner.right.unbounded) {
            region_discarding_ = region_t::empty();
        } else {
            region_discarding_.inner.left = region_queueing_.inner.right.key;
            region_discarding_.inner.right = store->get_region().inner.right;
        }
        std::queue<cond_t *> ack_queue;
        queue_fun = [&](queue_entry_t &&entry, cond_t *ack) {
            queue.push(std::move(entry));
            ack_queue.push(ack);
        };

        rwlock_acq.reset();

        /* Wait until we've received writes at least up to the latest point where the
        backfill left us. This ensures that we'll be able to safely ignore
        `backfill_end_timestamps` once we finish when draining the queue. */
        timestamp_enforcer_.wait_all_before(
            callback.backfill_end_timestamps.get_max_timestamp(), interruptor);

        /* Drain the queue. When the queue is empty, acquire the lock again, and then
        drain any further queue entries that were added while we were waiting on the
        lock. */
        auto_drainer_t queue_drainer;
        new_semaphore_t drain_semaphore(64);
        bool have_lock = false;
        while (true) {
            if (!queue.empty()) {
                new_semaphore_acq_t sem_acq(&drain_semaphore, interruptor);
                queue_entry_t entry = queue.pop();
                callback.backfill_end_timestamps.clip_write(
                    &entry.write, entry.timestamp);
                write_token_t token;
                store->new_write_token(&token);
                coro_t::spawn_sometime(std::bind(
                    [this, interruptor](queue_entry_t &&entry2, new_semaphore_acq_t &&,
                            auto_drainer_t::lock_t) {
                        try {
#ifndef NDEBUG
                            equality_metainfo_checker_callback_t checker_cb(
                                binary_blob_t(version_t(branch_id_, timestamp.pred())));
                            metainfo_checker_t checker(&checker_cb, region_queueing_);
#endif
                            write_response_t dummy_response;
                            store_->write(
                                DEBUG_ONLY(checker,)
                                region_map_t<binary_blob_t>(
                                    region_queueing_,
                                    binary_blob_t(version_t(branch_id, timestamp)),
                                subwrite_streaming,
                                &dummy_response,
                                write_durability_t::SOFT,
                                timestamp,
                                order_token,
                                &write_token,
                                interruptor);
                        } catch (const interrupted_exc_t &) {
                            /* ignore */
                        }
                    }, std::move(entry), std::move(sem_acq), queue_drainer.lock()));
            }
            if (queue.empty() && !have_lock) {
                rwlock_acq.init(new rwlock_acq_t(
                    &lock_regions, access_t::write, interruptor));
                have_lock = true;
            }
            if (queue.empty() && have_lock) {
                break;
            }
        }

        /* Make the region that was previously used for queueing instead be used for
        streaming */
        region_streaming_.inner.right = region_queueing_.inner.right;
        region_queueing_.inner = key_range_t::empty();
        backfill_end_timestamps_.combine(callback.backfill_end_timestamps);
    }

    replica_.reset(new replica_t(mailbox_manager_, store_, branch_history_manager,
        branch_id, timestamp_enforcer_.get_latest_all_before_completed()));

    rwlock_acq.reset();

    /* Tell the primary that it's OK to send us reads and synchronous writes */
    send(mailbox_manager, intro.ready_mailbox);
}

void remote_replicator_client_t::on_write_async(
        signal_t *interruptor,
        write_t &&write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        const mailbox_t<void()>::address_t &ack_addr)
        THROWS_NOTHING {
    wait_interruptible(&registered_, interruptor);
    timestamp_enforcer_->wait_all_before(timestamp, interruptor);

    rwlock_acq_t rwlock_acq(&lock_regions, access_t::read, interruptor);

    if (replica_.has()) {
        timestamp_enforcer_->complete(timestamp);
        rwlock_acq.reset();

        write_response_t dummy_response;
        replica_->do_write(write, timestamp, order_token, write_durability_t::SOFT,
            interruptor, &dummy_response);

    } else {
        queue_entry_t queue_entry;
        bool have_subwrite_queueing = write.shard(region_queueing, &queue_entry.write);
        cond_t queue_throttler;
        if (have_subwrite_queueing) {
            guarantee(queue_fun_ != nullptr);
            queue_entry.timestamp = timestamp;
            queue_entry.order_token = order_token;
            (*queue_fun)(std::move(queue_entry), &queue_unthrottled);
        }

        write_t subwrite_streaming;
        bool have_subwrite_streaming =
            write.shard(region_streaming_, &subwrite_streaming);
        write_token_t write_token;
        region_t region_streaming;
        if (have_subwrite_streaming) {
            store_->new_write_token(&write_token);
            region_streaming = region_streaming_;
        }

        rwlock_acq.reset();
        timestamp_enforcer_->complete(timestamp);

        if (have_subwrite_streaming) {
            region_t region_streaming = region_streaming_;
#ifndef NDEBUG
            equality_metainfo_checker_callback_t checker_cb(
                binary_blob_t(version_t(branch_id_, timestamp.pred())));
            metainfo_checker_t checker(&checker_cb, region_streaming);
#endif
            write_response_t dummy_response;
            store_->write(
                DEBUG_ONLY(checker,)
                region_map_t<binary_blob_t>(
                    region_streaming,
                    binary_blob_t(version_t(branch_id, timestamp)),
                subwrite_streaming,
                &dummy_response,
                write_durability_t::SOFT,
                timestamp,
                order_token,
                &write_token,
                interruptor);
        }

        if (have_subwrite_queueing) {
            wait_interruptible(&queue_throttler, interruptor);
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
        THROWS_NOTHING {
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
        THROWS_NOTHING {
    read_response_t response;
    replica_->do_read(read, min_timestamp, interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

