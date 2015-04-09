// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/remote_replicator_client.hpp"

#include "clustering/immediate_consistency/backfill_throttler.hpp"
#include "clustering/immediate_consistency/backfillee.hpp"
#include "store_view.hpp"

/* Maximum number of writes we can queue up in each cycle of the backfill. When we hit
this number, we'll interrupt the backfill and drain the queue. */
#define MAX_QUEUED_WRITES 1000

/* Every time we perform a queued write, we'll let `QUEUE_TRICKLE_FRACTION` new writes be
added to the back of the queue. So `QUEUE_TRICKLE_FRACTION` should be less than 1, to
ensure that the queue must drain eventually. Higher values will improve performance of
streaming writes; lower numbers will make the backfill finish faster. */
#define QUEUE_TRICKLE_FRACTION 0.5

/* Sometimes we'll receive the same write as part of our stream of writes from the
dispatcher and as part of our backfill from the backfiller. To avoid corruption, we need
to be sure that we don't apply the write twice. `backfill_end_timestamps_t` tracks which
writes were received as part of the backfill and filters the writes from the dispatcher
accordingly. This is tricky because sometimes a write will affect multiple keys, and
we'll only get half of it as part of the backfill; in this case, we still need to apply
the other half of the write we got from the dispatcher. */
class remote_replicator_client_t::backfill_end_timestamps_t {
public:
    backfill_end_timestamps_t() : region(region_t::empty()) { }

    /* `region_map` should be the timestamps of the store as of when the backfill
    completed. It assumes that the backfill timestamps increase as keys increase in
    lexicographical order. */
    backfill_end_timestamps_t(const region_map_t<state_timestamp_t> &region_map) {
        region = region_map.get_domain();
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

    /* If a write's timestamp is greater than `get_max_timestamp()`, there's no need for
    it to pass through `clip_write()`. */
    state_timestamp_t get_max_timestamp() const {
        return max_timestamp;
    }

    /* `clip_write()` checks if a write should be applied or not. If a write should not
    be applied, it returns `false`. If a write should be wholly or partially applied, it
    returns `true` and may alter `*w`. */
    bool clip_write(write_t *w, state_timestamp_t ts) const {
        rassert(region_is_superset(region, w->get_region()));
        if (ts > max_timestamp) {
            /* Shortcut for the simple case */
            return true;
        }
        region_t clip_region = w->get_region();
        for (const auto &step : steps) {
            if (key_range_t::right_bound_t(step.first) > clip_region.inner.right) {
                /* The write should be applied without modification, but if it had
                affected keys further to the right, that wouldn't have been the case. */
                return true;
            }
            if (step.second >= ts) {
                clip_region.inner.right = key_range_t::right_bound_t(step.first);
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

    /* `combine` concatenates two `backfill_end_timestamps_t`s that cover adjacent
    regions. */
    void combine(backfill_end_timestamps_t &&next) {
        if (region_is_empty(region)) {
            *this = std::move(next);
        } else {
            guarantee(region.beg == next.region.beg && region.end == next.region.end);
            guarantee(region.inner.right ==
                key_range_t::right_bound_t(next.region.inner.left));
            region.inner.right = next.region.inner.right;
            guarantee(steps.back().second <= next.steps.front().second);
            auto begin = next.steps.begin();
            if (steps.back().second == next.steps.front().second) {
                ++begin;
            }
            steps.insert(
                steps.end(),
                std::make_move_iterator(begin),
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
        /* RSI(raft): Respect backfill_throttler */
        UNUSED backfill_throttler_t *backfill_throttler,
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

    queue_fun_(nullptr),

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
    region_streaming_.inner = region_queueing_.inner = key_range_t::empty();

    /* Subscribe to the stream of writes coming from the primary */
    guarantee(remote_replicator_server_bcard.branch == branch_id);
    remote_replicator_client_intro_t intro;
    {
        remote_replicator_client_bcard_t::intro_mailbox_t intro_mailbox(
            mailbox_manager,
            [&](signal_t *, const remote_replicator_client_intro_t &i) {
                intro = i;
                timestamp_enforcer_.init(new timestamp_enforcer_t(
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
        replica_bcard.backfiller_bcard, interruptor);

    /* We acquire `rwlock_` to lock out writes while we're writing to `region_*_`,
    `queue_fun_`, and `replica_`, and for the last stage of draining the queue. */
    scoped_ptr_t<rwlock_acq_t> rwlock_acq(
        new rwlock_acq_t(&rwlock_, access_t::write, interruptor));

    while (region_streaming_.inner.right != store->get_region().inner.right) {
        /* Previously we were streaming some sub-range and discarding the rest. Here we
        leave the streaming region as it was but we start queueing the region we were
        previously discarding. */
        guarantee(region_queueing_.inner.is_empty());
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
            callback_t(std::queue<queue_entry_t> *_queue,
                    const key_range_t::right_bound_t &_right_bound) :
                queue(_queue), right_bound(_right_bound) { }
            bool on_progress(
                    const region_map_t<version_t> &chunk) {
                rassert(key_range_t::right_bound_t(chunk.get_domain().inner.left) ==
                    right_bound);
                right_bound = chunk.get_domain().inner.right;
                backfill_end_timestamps.combine(
                    region_map_transform<version_t, state_timestamp_t>(chunk,
                        [](const version_t &version) { return version.timestamp; }));
                return (queue->size() < MAX_QUEUED_WRITES);
            }
            std::queue<queue_entry_t> *queue;
            backfill_end_timestamps_t backfill_end_timestamps;
            key_range_t::right_bound_t right_bound;
        } callback(&queue, key_range_t::right_bound_t(region_queueing_.inner.left));

        backfillee.go(
            &callback,
            key_range_t::right_bound_t(region_queueing_.inner.left),
            interruptor);

        /* Wait until we've queued writes at least up to the latest point where the
        backfill left us. This ensures that it will be safe to ignore
        `backfill_end_timestamps` once we finish when draining the queue. */
        timestamp_enforcer_->wait_all_before(
            callback.backfill_end_timestamps.get_max_timestamp(), interruptor);

        rwlock_acq.init(new rwlock_acq_t(&rwlock_, access_t::write, interruptor));

        /* Shrink the queueing region to only contain the region that we just backfilled,
        and make anything to the right of that be the discarding region */
        region_queueing_.inner.right = callback.right_bound;
        if (region_queueing_.inner.right.unbounded) {
            region_discarding_ = region_t::empty();
        } else {
            region_discarding_.inner.left = region_queueing_.inner.right.key();
            region_discarding_.inner.right = store->get_region().inner.right;
        }

        /* As writes continue to come in, don't ack them immediately; instead put the
        ack conds into `acq_queue`. */
        std::queue<cond_t *> ack_queue;
        double acks_to_release = 0;
        queue_fun = [&](queue_entry_t &&entry, cond_t *ack) {
            queue.push(std::move(entry));
            if (acks_to_release >= 1) {
                acks_to_release -= 1;
                ack->pulse();
            } else {
                ack_queue.push(ack);
            }
        };

        rwlock_acq.reset();

        /* Drain the queue. */
        drain_stream_queue(
            store, branch_id, region_queueing_,
            &queue,
            callback.backfill_end_timestamps,
            /* This function will be called whenever the queue becomes empty. If the
            queue is still empty when it returns, then `drain_stream_queue()` will
            return. */
            [this, &rwlock_acq](signal_t *interruptor2) {
                /* When the queue first becomes empty, we acquire the lock. But while
                we're waiting for the lock, it's possible that more entries will be
                pushed onto the queue, so this might be called a second time. */
                if (!rwlock_acq.has()) {
                    rwlock_acq.init(
                        new rwlock_acq_t(&rwlock_, access_t::write, interruptor2));
                }
            },
            /* This function will be called whenever an entry from the stream queue has
            been written to the store */
            [&](signal_t *) {
                /* As we drain the main queue, we also pop entries off of `ack_queue`,
                but we pop fewer entries off of `ack_queue` than off of the main queue.
                This slows down the pace of incoming writes from the primary so that we
                can be sure that the queue will eventually drain. */
                acks_to_release += QUEUE_TRICKLE_FRACTION;
                if (acks_to_release >= 1 && !ack_queue.empty()) {
                    acks_to_release -= 1;
                    ack_queue.front()->pulse();
                    ack_queue.pop();
                }
            },
            interruptor);
        guarantee(rwlock_acq.has());
        guarantee(queue.empty());

        /* Now that the queue has completely drained, we're going to go back to allowing
        async writes to run without any throttling. So we should release any remaining
        writes that are waiting in `ack_queue`. */
        while (!ack_queue.empty()) {
            ack_queue.front()->pulse();
            ack_queue.pop();
        }

        /* Make the region that was previously used for queueing instead be used for
        streaming. We needed to completely drain the queue before making this transfer
        because there's no synchronization between streaming writes and queueing writes,
        so we can't move the region boundary until we're sure that all the writes in the
        queue have finished. */
        region_streaming_.inner.right = region_queueing_.inner.right;
        region_queueing_.inner = key_range_t::empty();
    }

#ifndef NDEBUG
    {
        /* Sanity check that the store's metainfo is all on the correct branch and all at
        the correct timestamp */
        region_map_t<binary_blob_t> metainfo_blob;
        read_token_t read_token;
        store->new_read_token(&read_token);
        store->do_get_metainfo(order_token_t::ignore.with_read_mode(),
            &read_token, interruptor, &metainfo_blob);
        for (const auto &pair : to_version_map(metainfo_blob)) {
            rassert(pair.second == version_t(branch_id,
                timestamp_enforcer_->get_latest_all_before_completed()));
        }
    }
#endif

    /* Now we're completely up-to-date and synchronized with the primary, it's time to
    create a `replica_t`. */
    replica_.init(new replica_t(mailbox_manager_, store_, branch_history_manager,
        branch_id, timestamp_enforcer_->get_latest_all_before_completed()));

    rwlock_acq.reset();

    /* Now that we're completely up-to-date, tell the primary that it's OK to send us
    reads and synchronous writes */
    send(mailbox_manager, intro.ready_mailbox);
}

void remote_replicator_client_t::drain_stream_queue(
        store_view_t *store,
        const branch_id_t &branch_id,
        const region_t &region,
        std::queue<queue_entry_t> *queue,
        const backfill_end_timestamps_t &bets,
        const std::function<void(signal_t *)> &on_queue_empty,
        const std::function<void(signal_t *)> &on_finished_one_entry,
        signal_t *interruptor) {
    auto_drainer_t drainer;
    new_semaphore_t semaphore(64);
    while (true) {
        /* If the queue is empty, notify our caller and give them a chance to put more
        things on the queue. If they don't, then we're done. */
        if (queue->empty()) {
            on_queue_empty(interruptor);
            if (queue->empty()) {
                break;
            }
        }

        /* Acquire the semaphore to limit how many coroutines we spawn concurrently. */
        scoped_ptr_t<new_semaphore_acq_t> sem_acq(
            new new_semaphore_acq_t(&semaphore, 1));
        wait_interruptible(sem_acq->acquisition_signal(), interruptor);

        scoped_ptr_t<queue_entry_t> entry(new queue_entry_t(queue->front()));
        queue->pop();
        bool metainfo_only = !bets.clip_write(&entry->write, entry->timestamp);

        /* Acquire a write token here rather than in the coroutine so that we can be sure
        the writes will acquire tokens in the correct order. */
        scoped_ptr_t<write_token_t> token(new write_token_t);
        store->new_write_token(token.get());

        auto_drainer_t::lock_t keepalive(&drainer);

        /* This lambda is a bit tricky. We want to capture `sem_acq`, `entry`, and
        `token` by "move", but that's impossible, so we convert them to raw pointers and
        capture those raw pointers by value. We want to capture `keepalive` and
        `metainfo_only` by value. Everything else we want to capture by reference,
        because it will outlive `drainer` and therefore outlive the coroutine. */
        new_semaphore_acq_t *sem_acq_ptr = sem_acq.release();
        queue_entry_t *entry_ptr = entry.release();
        write_token_t *token_ptr = token.release();
        coro_t::spawn_sometime([&branch_id, &interruptor, &on_finished_one_entry, &store,
                &region, sem_acq_ptr, entry_ptr, token_ptr, keepalive, metainfo_only]() {
            /* Immediately transfer the raw pointers back into `scoped_ptr_t`s to make
            sure that they get freed */
            scoped_ptr_t<new_semaphore_acq_t> sem_acq_2(sem_acq_ptr);
            scoped_ptr_t<queue_entry_t> entry_2(entry_ptr);
            scoped_ptr_t<write_token_t> token_2(token_ptr);
            try {
                /* Note that we don't block on the `auto_drainer_t::lock_t`'s drain
                signal. This way, `drain_stream_queue()` won't return until either all of
                the writes have been applied or the interruptor is pulsed. */
#ifndef NDEBUG
                equality_metainfo_checker_callback_t checker_cb(
                    binary_blob_t(version_t(branch_id, entry_2->timestamp.pred())));
                metainfo_checker_t checker(&checker_cb, entry_2->write.get_region());
#endif
                region_map_t<binary_blob_t> new_metainfo(
                    region, binary_blob_t(version_t(branch_id, entry_2->timestamp)));

                if (!metainfo_only) {
                    write_response_t dummy_response;
                    store->write(
                        DEBUG_ONLY(checker,)
                        new_metainfo,
                        entry_2->write,
                        &dummy_response,
                        write_durability_t::SOFT,
                        entry_2->timestamp,
                        order_token_t::ignore,
                        token_2.get(),
                        interruptor);
                } else {
                    store->set_metainfo(
                        new_metainfo,
                        order_token_t::ignore,
                        token_2.get(),
                        interruptor);
                }

                /* Notify the caller that we finished applying one write. The caller uses
                this to control how fast it adds writes to the queue, to be sure the
                queue will eventually drain. */
                on_finished_one_entry(interruptor);

            } catch (const interrupted_exc_t &) {
                /* ignore */
            }
        });
    }

    /* Block until all of the coroutines are finished */
    drainer.drain();

    /* It's possible that some of the coroutines aborted early because the interruptor
    was pulsed, so we need to check it here. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

void remote_replicator_client_t::on_write_async(
        signal_t *interruptor,
        write_t &&write,
        state_timestamp_t timestamp,
        order_token_t order_token,
        const mailbox_t<void()>::address_t &ack_addr)
        THROWS_NOTHING {
    wait_interruptible(&registered_, interruptor);
    timestamp_enforcer_->wait_all_before(timestamp.pred(), interruptor);

    rwlock_acq_t rwlock_acq(&rwlock_, access_t::read, interruptor);

    if (replica_.has()) {
        /* Once the constructor is done, all writes will take this branch; it's the
        common case. */
        timestamp_enforcer_->complete(timestamp);
        rwlock_acq.reset();

        write_response_t dummy_response;
        replica_->do_write(write, timestamp, order_token, write_durability_t::SOFT,
            interruptor, &dummy_response);

    } else {
        /* This branch is taken during the initial backfill. We need to break the write
        into three subwrites; the subwrite that applies to `region_streaming_`, the part
        that applies to `region_queueing_`, and the subwrite that applies to
        `region_discarding_`. We'll apply the first subwrite to the store immediately;
        pass the second subwrite to `queue_fun_`; and discard the third subwrite. Some of
        the subwrites may be empty. */

        write_t subwrite_streaming;
        bool have_subwrite_streaming =
            write.shard(region_streaming_, &subwrite_streaming);
        write_token_t write_token;
        region_t region_streaming;
        if (have_subwrite_streaming) {
            store_->new_write_token(&write_token);
            /* Make a local copy of `region_streaming_` because it might change once we
            release `rwlock_acq`. */
            region_streaming = region_streaming_;
        }

        queue_entry_t queue_entry;
        bool have_subwrite_queueing = write.shard(region_queueing_, &queue_entry.write);
        cond_t queue_throttler;
        if (have_subwrite_queueing) {
            guarantee(queue_fun_ != nullptr);
            queue_entry.timestamp = timestamp;
            queue_entry.order_token = order_token;
            (*queue_fun_)(std::move(queue_entry), &queue_throttler);
        }

        timestamp_enforcer_->complete(timestamp);
        rwlock_acq.reset();

        if (have_subwrite_streaming) {
#ifndef NDEBUG
            equality_metainfo_checker_callback_t checker_cb(
                binary_blob_t(version_t(branch_id_, timestamp.pred())));
            metainfo_checker_t checker(&checker_cb, region_streaming);
#endif
            write_response_t dummy_response;
            store_->write(
                DEBUG_ONLY(checker,)
                region_map_t<binary_blob_t>(
                    region_streaming_,
                    binary_blob_t(version_t(branch_id_, timestamp))),
                subwrite_streaming,
                &dummy_response,
                write_durability_t::SOFT,
                timestamp,
                order_token,
                &write_token,
                interruptor);
        }

        if (have_subwrite_queueing) {
            /* Wait until the queueing logic pulses our `queue_throttler`. The dispatcher
            will limit the number of outstanding writes to us at any given time; so if we
            delay acking this write, that will limit the rate at which the dispatcher
            sends us new writes. The constructor uses this to ensure that new writes
            enter the queue more slowly than writes are being removed from the queue. */
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
        THROWS_NOTHING {
    read_response_t response;
    replica_->do_read(read, min_timestamp, interruptor, &response);
    send(mailbox_manager_, ack_addr, response);
}

