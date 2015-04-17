// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfillee.hpp"

#include "arch/timing.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "concurrency/wait_any.hpp"

/* `ITEM_ACK_INTERVAL_MS` controls how often we send acknowledgements back to the
backfiller. If it's too short, we'll waste resources sending lots of tiny
acknowledgements; if it's too long, the pipeline might stall. */
static const int ITEM_ACK_INTERVAL_MS = 100;

/* `backfillee_t::session_t` contains all the bits and pieces for managing a single
backfill session. It's impossible to have multiple sessions running at once, so in
principle this could have been implemented as some member variables on `backfillee_t`;
but collecting the variables into an object makes it easier to reason about.

Here's how `backfillee_t` and `session_t` interact: `backfillee_t` creates the
`session_t`. The `session_t` is responsible for sending the begin-session, end-session,
and ack-items messages to the backfiller. Whenever the backfiller sends messages to the
`backfillee_t`'s mailboxes, the `backfillee_t` passes the messages on to the `session_t`
by calling `on_items()` and `on_ack_end_session()`. The `backfillee_t` calls
`wait_done()` in a separate coroutine to wait for the session to finish, then destroys
the session after it returns. If the backfillee is destroyed, then `wait_done()` is
interrupted and the backfillee calls the `session_t` destructor. */
class backfillee_t::session_t {
public:
    session_t(
            backfillee_t *_parent,
            const key_range_t::right_bound_t &_threshold,
            callback_t *_callback) :
        parent(_parent),
        threshold(_threshold),
        callback(_callback),
        callback_returned_false(false),
        items(_parent->store->get_region().beg, _parent->store->get_region().end,
            threshold),
        items_mem_size_unacked(0),
        sent_end_session(false),
        metainfo(region_map_t<version_t>::empty()),
        metainfo_binary(region_map_t<binary_blob_t>::empty()),
        pulse_when_items_arrive(nullptr)
    {
        send(parent->mailbox_manager, parent->intro.begin_session_mailbox,
            parent->fifo_source.enter_write(), threshold);
        coro_t::spawn_sometime(std::bind(
            &session_t::run, this, drainer.lock()));
    }

    /* `backfillee_t()` calls these callbacks when it receives messages from the
    backfiller via the corresponding mailboxes. */
    void on_items(
            region_map_t<version_t> &&metainfo_chunk,
            backfill_item_seq_t<backfill_item_t> &&chunk) {
        rassert(metainfo_chunk.get_domain() == chunk.get_region());
        items_mem_size_unacked += chunk.get_mem_size();
        items.concat(std::move(chunk));
        metainfo.extend_keys_right(std::move(metainfo_chunk));
        metainfo_binary = from_version_map(metainfo);
        if (pulse_when_items_arrive != nullptr) {
            pulse_when_items_arrive->pulse_if_not_already_pulsed();
        }
    }
    void on_ack_end_session() {
        guarantee(sent_end_session);
        got_ack_end_session.pulse();
    }

    void wait_done(signal_t *interruptor) {
        wait_interruptible(&done_cond, interruptor);
        guarantee(threshold == parent->store->get_region().inner.right
            || callback_returned_false);
        guarantee(got_ack_end_session.is_pulsed());
    }

private:
    /* `run()` runs in a separate coroutine for the duration of the session's existence.
    It does the main work of the session: draining backfill items from the `items` queue
    and passing them to the store. */
    void run(auto_drainer_t::lock_t keepalive) {
        with_priority_t p(CORO_PRIORITY_BACKFILL_RECEIVER);
        try {
            /* Loop until we reach the end of the backfill range. */
            while (threshold != parent->store->get_region().inner.right) {
                /* Wait until we receive some items from the backfiller so we have
                something to do, or the session is terminated. */
                while (items.empty_domain()) {
                    if (got_ack_end_session.is_pulsed()) {
                        /* The callback returned false, so we sent an end-session message
                        to the backfiller, and it replied; then we drained the `items`
                        queue. So this session is over. */
                        guarantee(callback_returned_false);
                        done_cond.pulse();
                        return;
                    }

                    /* Make sure that the backfiller isn't waiting for us to ack more
                    items */
                    send_ack_items();

                    /* `send_ack_items()` could block, so we have to check again */
                    if (!items.empty_domain()) {
                        break;
                    }

                    /* Wait for more items to arrive */
                    cond_t cond;
                    assignment_sentry_t<cond_t *> sentry(
                        &pulse_when_items_arrive, &cond);
                    wait_any_t waiter(&cond, &got_ack_end_session);
                    wait_interruptible(&waiter, keepalive.get_drain_signal());
                }

                /* Set up a `region_t` describing the range that still needs to be
                backfilled */
                region_t subregion = parent->store->get_region();
                subregion.inner.left = threshold.key();

                /* Copy items from `items` into the store until we finish the backfill
                range or we run out of items */
                class producer_t : public store_view_t::backfill_item_producer_t {
                public:
                    producer_t(session_t *_parent) : parent(_parent) {
                        coro_t::spawn_sometime(std::bind(
                            &producer_t::ack_periodically, this, drainer.lock()));
                    }
                    /* `next_item()`, `get_metainfo()`, and `on_commit()` will be called
                    by `receive_backfill()`. */
                    continue_bool_t next_item(
                            bool *is_item_out,
                            backfill_item_t *item_out,
                            key_range_t::right_bound_t *empty_range_out) THROWS_NOTHING {
                        if (!parent->items.empty_of_items()) {
                            /* This is the common case. */
                            *is_item_out = true;
                            *item_out = parent->items.front();
                            parent->items.pop_front();
                            return continue_bool_t::CONTINUE;
                        } else if (!parent->items.empty_domain()) {
                            /* There aren't any more items left in the queue, but there's
                            still a part of the key-space that we know there aren't any
                            items in yet. We can ask the store to apply the metainfo to
                            that part of the key-space. */
                            *is_item_out = false;
                            *empty_range_out = parent->items.get_right_key();
                            parent->items.delete_to_key(*empty_range_out);
                            return continue_bool_t::CONTINUE;
                        } else {
                            /* We ran out of items. Break out of `receive_backfill()` so
                            we can block and wait for more items. (We don't want to block
                            in this callback because the caller might be holding locks in
                            the B-tree.) */
                            return continue_bool_t::ABORT;
                        }
                    }
                    const region_map_t<binary_blob_t> *get_metainfo() THROWS_NOTHING {
                        return &parent->metainfo_binary;
                    }
                    void on_commit(const key_range_t::right_bound_t &new_threshold)
                            THROWS_NOTHING {
                        /* `on_commit()` might get called multiple times even after
                        `on_progress()` returns false, because calling
                        `send_end_session_message()` sets into motion a long chain of
                        events that eventually ends with the session being interrupted,
                        but might take a while. But we want to hide that complexity from
                        the callack, so we just skip calling `on_progress()` again if it
                        has returned `false` before. */
                        if (!parent->callback_returned_false) {
                            region_t mask = parent->parent->store->get_region();
                            mask.inner.left = parent->threshold.key();
                            mask.inner.right = new_threshold;
                            if (!parent->callback->on_progress(
                                    parent->metainfo.mask(mask))) {
                                parent->callback_returned_false = true;
                                parent->send_end_session_message();
                            }
                        }
                        parent->threshold = new_threshold;
                    }
                private:
                    /* `ack_periodically()` calls `session_t::send_ack_items()` every so
                    often during the backfill, so that the backfiller will keep sending
                    us items as they consume them and so ideally the `items` queue won't
                    ever bottom out before we're done. */
                    void ack_periodically(auto_drainer_t::lock_t keepalive2) {
                        try {
                            while (true) {
                                nap(ITEM_ACK_INTERVAL_MS, keepalive2.get_drain_signal());
                                parent->send_ack_items();
                            }
                        } catch (const interrupted_exc_t &) {
                            /* ignore */
                        }
                    }
                    session_t *parent;
                    auto_drainer_t drainer;
                } producer(this);

                parent->store->receive_backfill(
                    subregion, &producer, keepalive.get_drain_signal());
            }

            /* We reached the end of the range to be backfilled. The callback may or may
            not have returned `false` at some point along the way. */
            guarantee(items.empty_domain());

            /* Make sure that we acknowledged every single item that the backfiller sent
            us */
            send_ack_items();

            if (!callback_returned_false) {
                /* Do the handshake to end the session. It's a little bit redundant in
                this case (because the backfiller knows that it sent us items all the way
                to the end of the range, so we're not giving it any new information here)
                but we do it anyway just to be consistent. */
                send_end_session_message();
            } else {
                /* We already initiated the end-of-session handshake back when the
                callback first returned false. */
            }

            wait_interruptible(&got_ack_end_session, keepalive.get_drain_signal());
            guarantee(items.empty_domain());
            done_cond.pulse();

        } catch (const interrupted_exc_t &) {
            /* The backfillee was destroyed */
        }
    }

    /* `send_ack_items()` lets the backfiller know the total mem size of the items we've
    consumed since the last call to `send_ack_items()`, so it knows when it's safe to
    send more items. */
    void send_ack_items() {
        guarantee(items_mem_size_unacked >= items.get_mem_size());
        size_t diff = items_mem_size_unacked - items.get_mem_size();
        if (diff != 0) {
            items_mem_size_unacked -= diff;
            send(parent->mailbox_manager, parent->intro.ack_items_mailbox,
                parent->fifo_source.enter_write(), diff);
        }
    }

    void send_end_session_message() {
        guarantee(!sent_end_session);
        sent_end_session = true;
        send(parent->mailbox_manager, parent->intro.end_session_mailbox,
            parent->fifo_source.enter_write());
    }

    backfillee_t *const parent;

    /* `threshold` describes the current location that this session has reached. It
    starts at the threshold that was passed to `go()` and proceeds to the right. */
    key_range_t::right_bound_t threshold;

    callback_t *const callback;
    bool callback_returned_false;

    /* `items` is a queue containing all of the backfill items that we've received from
    the backfiller during this session. Incoming items are appended to the right-hand
    side; we consume items from the left-hand side and apply them to the B-tree. */
    backfill_item_seq_t<backfill_item_t> items;

    /* `items_mem_size_unacked` describes what the backfiller thinks the total mem size
    of `items` is. `send_ack_items()` uses this to calculate what to send. */
    size_t items_mem_size_unacked;

    /* `sent_end_session` is true if we've sent a message to the backfiller to end the
    session. `got_ack_end_session` is pulsed if we got an acknowledgement, so that no
    more items can possible be added to our queue. */
    bool sent_end_session;
    cond_t got_ack_end_session;

    /* `metainfo` contains all the metainfo we've received from the backfiller. It always
    extends from wherever we started the backfill to the right-hand side of `items`. */
    region_map_t<version_t> metainfo;

    /* `metainfo_binary` is just `metainfo` in `binary_blob_t` form. */
    region_map_t<binary_blob_t> metainfo_binary;

    /* `run()` puts a `cond_t` here when it needs to wait for more backfill items to be
    appended to `items`. `on_items()` pulses it when it appends more backfill items to
    `items`. */
    cond_t *pulse_when_items_arrive;

    /* `run()` pulses this when the session is completely over */
    cond_t done_cond;

    /* `drainer` must be destroyed before anything else, because it stops `run()`. */
    auto_drainer_t drainer;
};

backfillee_t::backfillee_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store,
        const backfiller_bcard_t &backfiller,
        const backfill_config_t &_backfill_config,
        signal_t *interruptor) :
    mailbox_manager(_mailbox_manager),
    branch_history_manager(_branch_history_manager),
    store(_store),
    backfill_config(_backfill_config),
    pre_item_throttler(backfill_config.pre_item_queue_mem_size),
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
    our_intro.config = backfill_config;
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
    /* Note: If we get interrupted during this function, then `current_session` will be
    left in place, which will make future calls to `go()` fail. So interrupting a call to
    `go()` invalidates the `backfillee_t`, and the only way to recover is to destroy the
    `backfillee_t`. Destroying the `backfillee_t` will destroy `current_session`, thereby
    interrupting whatever it is doing. */

    guarantee(!current_session.has());
    current_session.init(new session_t(this, threshold, callback));
    current_session->wait_done(interruptor);
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
    with_priority_t p(CORO_PRIORITY_BACKFILL_RECEIVER);
    try {
        key_range_t::right_bound_t pre_item_sent_threshold(
            store->get_region().inner.left);
        while (pre_item_sent_threshold != store->get_region().inner.right) {
            /* Wait until there's room in the semaphore for the chunk we're about to
            process */
            new_semaphore_acq_t sem_acq(
                &pre_item_throttler, backfill_config.pre_item_chunk_mem_size);
            wait_interruptible(
                sem_acq.acquisition_signal(), keepalive.get_drain_signal());

            /* Set up a `region_t` describing the range that still needs to be
            backfilled */
            region_t subregion = store->get_region();
            subregion.inner.left = pre_item_sent_threshold.key();

            /* Copy pre-items from the store into `chunk ` until the total size hits
            `backfill_config.pre_item_chunk_mem_size` or we finish the range */
            backfill_item_seq_t<backfill_pre_item_t> chunk(
                store->get_region().beg, store->get_region().end,
                pre_item_sent_threshold);

            class consumer_t : public store_view_t::backfill_pre_item_consumer_t {
            public:
                consumer_t(
                        backfill_item_seq_t<backfill_pre_item_t> *_chunk,
                        const backfill_config_t *_config) :
                    chunk(_chunk), config(_config) { }
                continue_bool_t on_pre_item(backfill_pre_item_t &&item) THROWS_NOTHING {
                    chunk->push_back(std::move(item));
                    if (chunk->get_mem_size() < config->pre_item_chunk_mem_size) {
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
                backfill_config_t const *const config;
            } callback(&chunk, &backfill_config);

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

