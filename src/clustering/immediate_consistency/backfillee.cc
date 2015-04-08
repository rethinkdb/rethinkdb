// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfillee.hpp"

#include "clustering/immediate_consistency/history.hpp"
#include "concurrency/wait_any.hpp"

/* `PRE_ITEM_PIPELINE_SIZE` is the maximum combined size of the pre-items that we send to
the backfiller that it hasn't consumed yet. So the other server may use up to this size
for its pre-item queue. `PRE_ITEM_CHUNK_SIZE` is the typical size of a pre-item message
we send over the network. */
static const size_t PRE_ITEM_PIPELINE_SIZE = 4 * MEGABYTE;
static const size_t PRE_ITEM_CHUNK_SIZE = 100 * KILOBYTE;

class backfillee_t::session_t {
public:
    session_t(
            backfillee_t *_parent,
            const key_range_t::right_bound_t &_threshold,
            callback_t *_callback) :
        parent(_parent), threshold(_threshold), callback(_callback),
        items(_parent->store->get_region().beg, _parent->store->get_region().end,
            threshold),
        metainfo(region_map_t<version_t>::empty()),
        metainfo_binary(region_map_t<binary_blob_t>::empty())
    {
        coro_t::spawn_sometime(std::bind(&session_t::run, this, drainer.lock()));
    }
    void on_items(
            region_map_t<version_t> &&metainfo_chunk,
            backfill_item_seq_t<backfill_item_t> &&chunk) {
        rassert(metainfo_chunk.get_domain() == chunk.get_region());
        items.concat(std::move(chunk));
        metainfo.concat(std::move(metainfo_chunk));
        metainfo_binary = from_version_map(metainfo);
        if (pulse_when_items_arrive.has()) {
            pulse_when_items_arrive->pulse_if_not_already_pulsed();
        }
    }
    void on_ack_end_session() {
        got_ack_end_session.pulse();
        if (pulse_when_items_arrive.has()) {
            pulse_when_items_arrive->pulse_if_not_already_pulsed();
        }
    }
    cond_t callback_returned_false;
    cond_t got_ack_end_session;
    cond_t run_stopped;

private:
    void run(auto_drainer_t::lock_t keepalive) {
        try {
            while (threshold != parent->store->get_region().inner.right &&
                    !got_ack_end_session.is_pulsed()) {
                /* Set up a `region_t` describing the range that still needs to be
                backfilled */
                region_t subregion = parent->store->get_region();
                subregion.inner.left = threshold.key();

                /* Copy items from `items` into the store until we finish the backfill
                range or we run out of items */
                class producer_t : public store_view_t::backfill_item_producer_t {
                public:
                    producer_t(session_t *_parent) : parent(_parent) { }
                    continue_bool_t next_item(
                            bool *is_item_out,
                            backfill_item_t *item_out,
                            key_range_t::right_bound_t *edge_out) THROWS_NOTHING {
                        if (!parent->items.empty()) {
                            *is_item_out = true;
                            *item_out = parent->items.front();
                            parent->items.pop_front();
                            return continue_bool_t::CONTINUE;
                        } else if (parent->items.get_left_key() <=
                                parent->items.get_right_key()) {
                            *is_item_out = false;
                            *edge_out = parent->items.get_right_key();
                            parent->items.delete_to_key(*edge_out);
                            return continue_bool_t::CONTINUE;
                        } else {
                            parent->pulse_when_items_arrive.init(new cond_t);
                            return continue_bool_t::ABORT;
                        }
                    }
                    const region_map_t<binary_blob_t> *get_metainfo() THROWS_NOTHING {
                        return &parent->metainfo_binary;
                    }
                    void on_commit(const key_range_t::right_bound_t &new_threshold)
                            THROWS_NOTHING {
                        if (!parent->callback_returned_false.is_pulsed()) {
                            region_t mask = parent->parent->store->get_region();
                            mask.inner.left = parent->threshold.key();
                            mask.inner.right = new_threshold;
                            if (!parent->callback->on_progress(
                                    parent->metainfo.mask(mask))) {
                                parent->callback_returned_false.pulse();
                            }
                        }
                        parent->threshold = new_threshold;
                    }
                    session_t *parent;
                } producer(this);

                parent->store->receive_backfill(
                    subregion, &producer, keepalive.get_drain_signal());

                /* Wait for more items, if that's the reason we stopped */
                if (pulse_when_items_arrive.has()) {
                    wait_interruptible(pulse_when_items_arrive.get(),
                        keepalive.get_drain_signal());
                    pulse_when_items_arrive.reset();
                }
            }
        } catch (const interrupted_exc_t &) {
            /* The backfillee was destroyed */
        }
        run_stopped.pulse();
    }
    backfillee_t *parent;
    key_range_t::right_bound_t threshold;
    callback_t *callback;
    backfill_item_seq_t<backfill_item_t> items;
    region_map_t<version_t> metainfo;
    region_map_t<binary_blob_t> metainfo_binary;
    scoped_ptr_t<cond_t> pulse_when_items_arrive;
    auto_drainer_t drainer;
};

backfillee_t::backfillee_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store,
        const backfiller_bcard_t &backfiller,
        signal_t *interruptor) :
    mailbox_manager(_mailbox_manager),
    branch_history_manager(_branch_history_manager),
    store(_store),
    pre_item_throttler(PRE_ITEM_PIPELINE_SIZE),
    pre_item_throttler_acq(&pre_item_throttler, 0),
    items_mailbox(mailbox_manager,
        std::bind(&backfillee_t::on_items, this, ph::_1, ph::_2, ph::_3, ph::_4)),
    ack_end_session_mailbox(mailbox_manager,
        std::bind(&backfillee_t::on_ack_end_session, this, ph::_1, ph::_2)),
    ack_pre_items_mailbox(mailbox_manager,
        std::bind(&backfillee_t::on_ack_pre_items, this, ph::_1, ph::_2, ph::_3))
{
    guarantee(region_is_superset(backfiller.region, store->get_region()));
    guarantee(store->get_region().beg == backfiller.region.beg);
    guarantee(store->get_region().end == backfiller.region.end);

    backfiller_bcard_t::intro_1_t our_intro;
    our_intro.ack_pre_items_mailbox = ack_pre_items_mailbox.get_address();
    our_intro.items_mailbox = items_mailbox.get_address();
    our_intro.ack_end_session_mailbox = ack_end_session_mailbox.get_address();

    /* Fetch the `initial_version` and `initial_version_history` fields for the
    `intro_1_t` that we'll send to the backfiller */
    {
        region_map_t<binary_blob_t> initial_state_blob;
        read_token_t read_token;
        store->new_read_token(&read_token);
        store->do_get_metainfo(order_token_t::ignore.with_read_mode(), &read_token,
            interruptor, &initial_state_blob);
        our_intro.initial_version = to_version_map(initial_state_blob);
        branch_history_manager->export_branch_history(
            our_intro.initial_version,
            &our_intro.initial_version_history);
    }

    /* Set up a mailbox to receive the `intro_2_t` from the backfiller */
    cond_t got_intro;
    mailbox_t<void(backfiller_bcard_t::intro_2_t)> intro_mailbox(
        mailbox_manager,
        [&](signal_t *, const backfiller_bcard_t::intro_2_t &i) {
            intro = i;
            got_intro.pulse();
        });
    our_intro.intro_mailbox = intro_mailbox.get_address();

    /* Send the `intro_1_t` to the backfiller and wait for it to send back the
    `intro_2_t` */
    registrant.init(new registrant_t<backfiller_bcard_t::intro_1_t>(
        mailbox_manager, backfiller.registrar, our_intro));
    wait_interruptible(&got_intro, interruptor);

    /* Spawn the coroutine that will stream pre-items to the backfiller. */
    coro_t::spawn_sometime(
        std::bind(&backfillee_t::send_pre_items, this, drainer.lock()));
}

backfillee_t::~backfillee_t() {
    /* This destructor is declared in the `.cc` file because we need to have the full
    definition of `session_t` in scope for the `scoped_ptr_t<session_t>` to work. */
}

void backfillee_t::go(
        callback_t *callback,
        const key_range_t::right_bound_t &threshold,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    guarantee(!current_session.has());
    current_session.init(new session_t(this, threshold, callback));

    send(mailbox_manager, intro.begin_session_mailbox,
        fifo_source.enter_write(), threshold);

    {
        wait_any_t combiner(
            &current_session->callback_returned_false, &current_session->run_stopped);
        wait_interruptible(&combiner, interruptor);
    }

    send(mailbox_manager, intro.end_session_mailbox,
        fifo_source.enter_write());

    wait_interruptible(&current_session->got_ack_end_session, interruptor);

    current_session.reset();
}

void backfillee_t::on_items(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        region_map_t<version_t> &&version,
        backfill_item_seq_t<backfill_item_t> &&chunk) {
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, fifo_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(current_session.has());
    current_session->on_items(std::move(version), std::move(chunk));
}

void backfillee_t::on_ack_end_session(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token) {
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, fifo_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(current_session.has());
    current_session->on_ack_end_session();
}

void backfillee_t::send_pre_items(auto_drainer_t::lock_t keepalive) {
    try {
        key_range_t::right_bound_t pre_item_sent_threshold(
            store->get_region().inner.left);
        while (pre_item_sent_threshold != store->get_region().inner.right) {
            /* Wait until there's room in the semaphore for the chunk we're about to
            process */
            new_semaphore_acq_t sem_acq(&pre_item_throttler, PRE_ITEM_CHUNK_SIZE);
            wait_interruptible(
                sem_acq.acquisition_signal(), keepalive.get_drain_signal());

            /* Set up a `region_t` describing the range that still needs to be
            backfilled */
            region_t subregion = store->get_region();
            subregion.inner.left = pre_item_sent_threshold.key();

            /* Copy pre-items from the store into `chunk ` until the total size hits
            `PRE_ITEM_CHUNK_SIZE` or we finish the range */
            backfill_item_seq_t<backfill_pre_item_t> chunk(
                store->get_region().beg, store->get_region().end,
                pre_item_sent_threshold);

            class consumer_t : public store_view_t::backfill_pre_item_consumer_t {
            public:
                consumer_t(backfill_item_seq_t<backfill_pre_item_t> *_chunk) :
                    chunk(_chunk) { }
                continue_bool_t on_pre_item(backfill_pre_item_t &&item) THROWS_NOTHING {
                    chunk->push_back(std::move(item));
                    if (chunk->get_mem_size() < PRE_ITEM_CHUNK_SIZE) {
                        return continue_bool_t::CONTINUE;
                    } else {
                        return continue_bool_t::ABORT;
                    }
                }
                continue_bool_t on_empty_range(
                        const key_range_t::right_bound_t &threshold) THROWS_NOTHING {
                    chunk->push_back_nothing(threshold);
                    return continue_bool_t::CONTINUE;
                }
                backfill_item_seq_t<backfill_pre_item_t> *chunk;
            } callback(&chunk);

            store->send_backfill_pre(intro.common_version.mask(subregion), &callback,
                keepalive.get_drain_signal());

            /* Adjust for the fact that `chunk.get_mem_size()` isn't precisely equal to
            `PRE_ITEM_CHUNK_SIZE`, and then transfer the semaphore ownership. */
            sem_acq.change_count(chunk.get_mem_size());
            pre_item_throttler_acq.transfer_in(std::move(sem_acq));

            /* Send the chunk over the network */
            send(mailbox_manager, intro.pre_items_mailbox,
                fifo_source.enter_write(), chunk);

            /* Update `progress` */
            guarantee(chunk.get_left_key() == pre_item_sent_threshold);
            pre_item_sent_threshold = chunk.get_right_key();
        }
    } catch (const interrupted_exc_t &) {
        /* The `backfillee_t` was deleted; the backfill is being aborted. */
    }
}

void backfillee_t::on_ack_pre_items(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        size_t mem_size) {
    /* Ordering of these messages is actually irrelevant; we just pass through the fifo
    sink for consistency */
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, fifo_token);
    wait_interruptible(&exit_write, interruptor);

    /* Shrink `pre_item_throttler_acq` so that `send_pre_items()` can acquire the
    semaphore again. */
    guarantee(static_cast<int64_t>(mem_size) <= pre_item_throttler_acq.count());
    pre_item_throttler_acq.change_count(pre_item_throttler_acq.count() - mem_size);
}

