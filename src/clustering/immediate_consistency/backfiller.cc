// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfiller.hpp"

#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"
#include "store_view.hpp"

/* `ITEM_PIPELINE_SIZE` is the maximum combined size of the items that we send to the
backfillee that it hasn't consumed yet. `ITEM_CHUNK_SIZE` is the typical size of an item
message we send over the network. */
static const size_t ITEM_PIPELINE_SIZE = 4 * MEGABYTE;
static const size_t ITEM_CHUNK_SIZE = 100 * KILOBYTE;

backfiller_t::backfiller_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store) :
    mailbox_manager(_mailbox_manager),
    branch_history_manager(_branch_history_manager),
    store(_store),
    registrar(mailbox_manager, this)
    { }

backfiller_t::client_t::client_t(
        backfiller_t *_parent,
        const backfiller_bcard_t::intro_1_t &_intro,
        signal_t *interruptor) :
    parent(_parent),
    intro(_intro),
    full_region(intro.initial_version.get_domain()),
    pre_items(full_region.beg, full_region.end,
        key_range_t::right_bound_t(full_region.inner.left)),
    item_throttler(ITEM_PIPELINE_SIZE),
    item_throttler_acq(&item_throttler, 0),
    pre_items_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_pre_items, this, ph::_1, ph::_2, ph::_3)),
    begin_session_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_begin_session, this, ph::_1, ph::_2, ph::_3)),
    end_session_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_end_session, this, ph::_1, ph::_2)),
    ack_items_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_ack_items, this, ph::_1, ph::_2, ph::_3))
{
    /* Compute the common ancestor of our version and the backfillee's version */
    {
        region_map_t<binary_blob_t> our_version_blob;
        read_token_t read_token;
        parent->store->new_read_token(&read_token);
        parent->store->do_get_metainfo(order_token_t::ignore.with_read_mode(),
            &read_token, interruptor, &our_version_blob);
        region_map_t<version_t> our_version = to_version_map(our_version_blob);

        branch_history_combiner_t combined_history(
            parent->branch_history_manager,
            &intro.initial_version_history);

        std::vector<std::pair<region_t, state_timestamp_t> > common_pairs;
        for (const auto &pair1 : our_version) {
            for (const auto &pair2 : intro.initial_version.mask(pair1.first)) {
                for (const auto &pair3 : version_find_common(&combined_history,
                        pair1.second, pair2.second, pair2.first)) {
                    common_pairs.push_back(
                        std::make_pair(pair3.first, pair3.second.timestamp));
                }
            }
        }
        common_version = region_map_t<state_timestamp_t>(
            common_pairs.begin(), common_pairs.end());
    }

    /* Send the computed common ancestor to the backfillee, along with the mailboxes it
    can use to contact us */
    backfiller_bcard_t::intro_2_t our_intro;
    our_intro.common_version = common_version;
    our_intro.pre_items_mailbox = pre_items_mailbox.get_address();
    our_intro.begin_session_mailbox = begin_session_mailbox.get_address();
    our_intro.end_session_mailbox = end_session_mailbox.get_address();
    our_intro.ack_items_mailbox = ack_items_mailbox.get_address();
    send(parent->mailbox_manager, intro.intro_mailbox, our_intro);
}

/* `item_seq_pre_item_producer_t` is a `backfill_pre_item_producer_t` that reads from a
`backfill_item_seq_t<backfill_pre_item_t>`. It keeps track of its position in the seq by
moving the elements of the seq to an internal seq; it restores the seq to its original
state when it goes out of scope. */
class item_seq_pre_item_producer_t : public store_view_t::backfill_pre_item_producer_t {
public:
    item_seq_pre_item_producer_t(backfill_item_seq_t<backfill_pre_item_t> *_pi) :
        pre_items(_pi),
        temp_buf(_pi->get_beg_hash(), _pi->get_end_hash(), _pi->get_left_key())
        { }
    ~item_seq_pre_item_producer_t() {
        temp_buf.concat(std::move(*pre_items));
        *pre_items = std::move(temp_buf);
    }
    continue_bool_t next_pre_item(
            bool *is_item_out,
            backfill_pre_item_t const **item_out,
            key_range_t::right_bound_t *empty_range_out)
            THROWS_NOTHING {
        if (!pre_items->empty_of_items()) {
            /* This is the common case. */
            *is_item_out = true;
            *item_out = &pre_items->front();
            return continue_bool_t::CONTINUE;
        } else if (!pre_items->empty_domain()) {
            /* There aren't any more pre-items left in the queue, but there's still a
            part of the key-space that we know there aren't any items in yet. We can ask
            the store to backfill us items from that space. */
            *is_item_out = false;
            *empty_range_out = pre_items->get_right_key();
            pre_items->delete_to_key(*empty_range_out);
            temp_buf.push_back_nothing(*empty_range_out);
            return continue_bool_t::CONTINUE;
        } else {
            /* We ran out of pre-items. Break out of `send_backfill()` so we can block
            and wait for more items. (We don't want to block in this callback because the
            caller might be holding locks in the B-tree.) */
            return continue_bool_t::ABORT;
        }
    }
    void release_pre_item() THROWS_NOTHING {
        pre_items->pop_front_into(&temp_buf);
    }
private:
    /* `pre_items` is the sequence we're reading from */
    backfill_item_seq_t<backfill_pre_item_t> *pre_items;

    /* Temporary storage for pre-items that have been consumed */
    backfill_item_seq_t<backfill_pre_item_t> temp_buf;
};

/* When the `client_t` receives a begin-session message from the backfillee, it creates a
`session_t`. The `session_t` is responsible for sending items to the backfillee. When the
session is over, the backfillee will send an end-session message to the `client_t`, which
will destroy the `session_t` and then send an ack-end-session message back to the
backfillee. */
class backfiller_t::client_t::session_t {
public:
    session_t(client_t *_parent, const key_range_t::right_bound_t &_threshold) :
        parent(_parent), threshold(_threshold)
    {
        coro_t::spawn_sometime(std::bind(&session_t::run, this, drainer.lock()));
    }

    /* Every time the `client_t` receives more pre-items from the backfillee, it calls
    `on_pre_items()` to notify us. */
    void on_pre_items() {
        if (pulse_when_pre_items_arrive.has()) {
            pulse_when_pre_items_arrive->pulse_if_not_already_pulsed();
        }
    }

private:
    void run(auto_drainer_t::lock_t keepalive) {
        try {
            while (threshold != parent->full_region.inner.right) {
                /* Wait until there's room in the semaphore for the chunk we're about to
                process */
                new_semaphore_acq_t sem_acq(&parent->item_throttler, ITEM_CHUNK_SIZE);
                wait_interruptible(
                    sem_acq.acquisition_signal(), keepalive.get_drain_signal());

                /* Wait until we have some pre items, or else we won't be able to make
                any progress on the backfill */
                guarantee(parent->pre_items.get_left_key() == threshold);
                while (parent->pre_items.empty_domain()) {
                    pulse_when_pre_items_arrive.init(new cond_t);
                    wait_interruptible(pulse_when_pre_items_arrive.get(),
                        keepalive.get_drain_signal());
                    pulse_when_pre_items_arrive.reset();
                }

                /* Set up a `region_t` describing the range that still needs to be
                backfilled */
                region_t subregion = parent->full_region;
                subregion.inner.left = threshold.key();

                /* Copy items from the store into `chunk` until the total size hits
                `ITEM_CHUNK_SIZE`; we finish the backfill range; or we run out of
                pre-items. */

                backfill_item_seq_t<backfill_item_t> chunk(
                    parent->full_region.beg, parent->full_region.end,
                    threshold);
                region_map_t<version_t> metainfo = region_map_t<version_t>::empty();

                {
                    /* `producer` will pop items off of `parent->pre_items` as
                    `send_backfill()` requests them, but it will restore them all when it
                    goes out of scope. Restoring them is important because when we
                    interrupt `send_backfill()`, it might not yet have produced items
                    corresponding to all of the pre-items it consumed, so those pre-items
                    might be needed again in a later call to `send_backfill()`. It's a
                    little sketchy that `producer` modifies `pre_items` like this, but
                    it's safe because only one `session_t` can exist at once and because
                    `client_t::on_pre_items` only touches the right-hand end of
                    `parent->pre_items`. */
                    item_seq_pre_item_producer_t producer(&parent->pre_items);

                    /* `consumer_t` is responsible for receiving backfill items and
                    the corresponding metainfo from `send_backfill()` and storing them in
                    `chunk` and in `metainfo`. */
                    class consumer_t : public store_view_t::backfill_item_consumer_t {
                    public:
                        consumer_t(backfill_item_seq_t<backfill_item_t> *_chunk,
                                region_map_t<version_t> *_metainfo) :
                            chunk(_chunk), metainfo(_metainfo) { }
                        continue_bool_t on_item(
                                const region_map_t<binary_blob_t> &item_metainfo,
                                backfill_item_t &&item) THROWS_NOTHING {
                            rassert(key_range_t::right_bound_t(item.range.left) >=
                                chunk->get_right_key());
                            rassert(!item.range.is_empty());
                            on_metainfo(item_metainfo, item.range.right);
                            chunk->push_back(std::move(item));
                            if (chunk->get_mem_size() < ITEM_CHUNK_SIZE) {
                                return continue_bool_t::CONTINUE;
                            } else {
                                return continue_bool_t::ABORT;
                            }
                        }
                        continue_bool_t on_empty_range(
                                const region_map_t<binary_blob_t> &range_metainfo,
                                const key_range_t::right_bound_t &new_threshold)
                                THROWS_NOTHING {
                            rassert(new_threshold >= chunk->get_right_key());
                            on_metainfo(range_metainfo, new_threshold);
                            chunk->push_back_nothing(new_threshold);
                            return continue_bool_t::CONTINUE;
                        }
                    private:
                        void on_metainfo(
                                const region_map_t<binary_blob_t> &new_metainfo,
                                const key_range_t::right_bound_t &new_threshold) {
                            if (new_threshold == chunk->get_right_key()) {
                                /* This is a no-op. But if `chunk->get_right_key()` is
                                already unbounded, calling `key()` on it will crash. So
                                the normal code path might break on certain no-ops and we
                                should just return instead. */
                                return;
                            }
                            region_t mask;
                            mask.beg = chunk->get_beg_hash();
                            mask.end = chunk->get_end_hash();
                            mask.inner.left = chunk->get_right_key().key();
                            mask.inner.right = new_threshold;
                            metainfo->concat(to_version_map(new_metainfo.mask(mask)));
                        }
                        backfill_item_seq_t<backfill_item_t> *chunk;
                        region_map_t<version_t> *metainfo;
                    } consumer(&chunk, &metainfo);

                    parent->parent->store->send_backfill(
                        parent->common_version.mask(subregion), &producer, &consumer,
                        keepalive.get_drain_signal());

                    /* `producer` goes out of scope here, so it restores `pre_items` to
                    what it was before */
                }

                /* Check if we actually got a non-empty chunk; if we got an empty chunk
                there's no point in sending it over the network. Note that we use
                `empty_domain()` instead of `empty_of_items()`, because the knowledge
                that there are no backfill items in the given range is still very useful
                information for the backfillee to have. */
                if (!chunk.empty_domain()) {
                    /* Adjust for the fact that `chunk.get_mem_size()` isn't precisely
                    equal to `ITEM_CHUNK_SIZE`, and then transfer the semaphore
                    ownership. */
                    sem_acq.change_count(chunk.get_mem_size());
                    parent->item_throttler_acq.transfer_in(std::move(sem_acq));

                    /* Update `threshold` */
                    guarantee(chunk.get_left_key() == threshold);
                    threshold = chunk.get_right_key();

                    /* Note: It's essential that we update `common_version` and
                    `pre_items` if and only if we send the chunk over the network. So
                    we shouldn't e.g. check the interruptor in between. This is because
                    we want to make sure that `common_version` and `pre_items` accurately
                    represent the state of the backfillee after it applies all the chunks
                    we've sent. */
                    try {
                        /* Send the chunk over the network */
                        send(parent->parent->mailbox_manager,
                            parent->intro.items_mailbox,
                            parent->fifo_source.enter_write(), metainfo, chunk);

                        /* Update `common_version` to reflect the changes that will
                        happen on the backfillee in response to the chunk */
                        parent->common_version.update(
                            region_map_transform<version_t, state_timestamp_t>(
                                metainfo,
                                [](const version_t &v) { return v.timestamp; }));

                        /* Discard pre-items we don't need anymore. This has two
                        purposes: it saves memory, and it keeps `pre_items` consistent
                        with `common_version`. */
                        size_t old_size = parent->pre_items.get_mem_size();
                        parent->pre_items.delete_to_key(threshold);
                        size_t new_size = parent->pre_items.get_mem_size();

                        /* Notify the backfiller that it's OK to send us more pre items.

                        Note that the way we acknowledge pre-items is different from how
                        the backfillee acknowledges items; the backfillee acknowledges
                        items periodically during the call to `receive_backfill()`, but
                        we wait until after `send_backfill()` returns to acknowledge the
                        pre-items. This way of doing things is simpler; the reason we
                        can't do things the same way in the backfillee is because the
                        call to `receive_backfill()` might run for a long time, but our
                        call to `send_backfill()` will only go until it fills up the
                        chunk. */
                        if (old_size != new_size) {
                            send(parent->parent->mailbox_manager,
                                parent->intro.ack_pre_items_mailbox,
                                parent->fifo_source.enter_write(), old_size - new_size);
                        }
                    } catch (const interrupted_exc_t &) {
                        /* This is just a sanity check in case someone unthinkingly makes
                        something interruptible in the above code block */
                        crash("We shouldn't be interrupted during this block");
                    }
                }
            }
        } catch (const interrupted_exc_t &) {
            /* The backfillee sent us a stop message; or the backfillee was destroyed; or
            the backfiller was destroyed. */
        }
    }

    client_t *const parent;

    /* This is the current location we've backfilled up to. Initially it's set to the
    point that the backfillee sent us with the begin-session message, and it moves right
    from there. */
    key_range_t::right_bound_t threshold;

    /* This exists if we're waiting for more pre items. `on_pre_items()` pulses it if it
    exists. */
    scoped_ptr_t<cond_t> pulse_when_pre_items_arrive;

    /* Destructor order matters here: `drainer` must be destroyed before the other member
    variables because `drainer` stops `run()`, which accesses the other member
    variables. */
    auto_drainer_t drainer;
};

void backfiller_t::client_t::on_begin_session(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const key_range_t::right_bound_t &threshold) {
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(threshold <= pre_items.get_left_key(), "Every key must be backfilled at "
        "least once; it's not OK for sessions to have gaps between them.");

    guarantee(!current_session.has());
    current_session.init(new session_t(this, threshold));
}

void backfiller_t::client_t::on_end_session(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token) {
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(current_session.has());
    current_session.reset();

    /* `session_t`'s destructor won't return until it's done sending items over the
    network, so we can be sure that the ack-end-session message comes after all of the
    items that were part of the session. */
    send(parent->mailbox_manager, intro.ack_end_session_mailbox,
        fifo_source.enter_write());
}

void backfiller_t::client_t::on_ack_items(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        size_t mem_size) {
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(static_cast<int64_t>(mem_size) <= item_throttler_acq.count());
    item_throttler_acq.change_count(item_throttler_acq.count() - mem_size);
}

void backfiller_t::client_t::on_pre_items(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        backfill_item_seq_t<backfill_pre_item_t> &&chunk) {
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    pre_items.concat(std::move(chunk));
    if (current_session.has()) {
        current_session->on_pre_items();
    }
}

