// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfiller.hpp"

#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"
#include "store_view.hpp"

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
    item_throttler(intro.config.item_queue_mem_size),
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
    /* Fetch our current state from the superblock metainfo */
    region_map_t<version_t> our_version;
    {
        read_token_t read_token;
        parent->store->new_read_token(&read_token);
        our_version = to_version_map(parent->store->get_metainfo(
            order_token_t::ignore.with_read_mode(), &read_token,
            parent->store->get_region(), interruptor));
    }

    /* Compute the common ancestor of `intro.initial_version` and `our_version`, storing
    it in `common_version`. And while we're on the branch history manager's thread,
    retrieve the branch history for `our_version`. */
    branch_history_t our_version_history;
    {
        on_thread_t thread_switcher(parent->branch_history_manager->home_thread());

        branch_history_combiner_t combined_history(
            parent->branch_history_manager,
            &intro.initial_version_history);

        common_version = intro.initial_version.map_multi(
            intro.initial_version.get_domain(),
            [&](const region_t &region1, const version_t &version1) {
                return our_version.map_multi(region1,
                    [&](const region_t &region2, const version_t &version2) {
                        try {
                            return version_find_common(
                                    &combined_history, version1, version2, region2)
                                .map(region2,
                                    [](const version_t &v) { return v.timestamp; });
                        } catch (const missing_branch_exc_t &) {
                            /* If we don't have enough information to determine the
                            common ancestor of the two versions, act as if it's zero.
                            This will always produce correct results but it might make us
                            do more backfilling than necessary. */
                            return region_map_t<state_timestamp_t>(
                                region2, state_timestamp_t::zero());
                        }
                    });
            });

        /* Also copy out the current value of the branch history. We only send the branch
        history once at the beginning of the backfill; this is OK because the underlying
        store isn't allowed to transition to a new branch while the `backfiller_t`
        exists. */
        parent->branch_history_manager->export_branch_history(
            our_version, &our_version_history);
    }

    /* Fetch the key distribution from the store, this is used by the backfillee to
    calculate the progress of backfill jobs. */
    std::map<store_key_t, int64_t> distribution_counts;
    int64_t distribution_counts_sum = 0;
    {
        static const int max_depth = 2;
        static const size_t result_limit = 128;

#ifndef NDEBUG
        metainfo_checker_t metainfo_checker(
            parent->store->get_region(),
            [](const region_t &, const binary_blob_t &) { });
#endif

        distribution_read_t distribution_read(max_depth, result_limit);
        read_t read(
            distribution_read, profile_bool_t::DONT_PROFILE, read_mode_t::OUTDATED);
        read_response_t read_response;
        read_token_t read_token;
        parent->store->read(
            DEBUG_ONLY(metainfo_checker, )
            read, &read_response, &read_token, interruptor);
        distribution_counts = std::move(
            boost::get<distribution_read_response_t>(read_response.response).key_counts);

        /* For the progress calculation we need partial sums for each key thus we
        calculate those from the results that the distribution query returns. */
        for (auto &&distribution_count : distribution_counts) {
            distribution_count.second =
                (distribution_counts_sum += distribution_count.second);
        }
    }

    /* Estimate the total number of changes that will need to be backfilled, by comparing
    `our_version`, `intro.initial_version`, and `common_version`. We estimate the number
    of changes as the largest version difference. In theory we could be smarter by
    cross-referencing with `distribution_counts`, but it's not worth the trouble for now.
    */
    uint64_t num_changes_estimate = 0;
    our_version.visit(full_region, [&](const region_t &r1, const version_t &v1) {
        intro.initial_version.visit(r1, [&](const region_t &r2, const version_t &v2) {
            common_version.visit(r2, [&](const region_t &, const state_timestamp_t &b) {
                guarantee(v1.timestamp >= b);
                guarantee(v2.timestamp >= b);
                uint64_t backfiller_changes = v1.timestamp.count_changes(b);
                uint64_t backfillee_changes = v2.timestamp.count_changes(b);
                uint64_t total_changes = backfiller_changes + backfillee_changes;
                num_changes_estimate = std::max(num_changes_estimate, total_changes);
            });
        });
    });

    /* Send the computed common ancestor to the backfillee, along with the mailboxes it
    can use to contact us. */
    backfiller_bcard_t::intro_2_t our_intro;
    our_intro.common_version = common_version;
    our_intro.final_version_history = std::move(our_version_history);
    our_intro.pre_items_mailbox = pre_items_mailbox.get_address();
    our_intro.begin_session_mailbox = begin_session_mailbox.get_address();
    our_intro.end_session_mailbox = end_session_mailbox.get_address();
    our_intro.ack_items_mailbox = ack_items_mailbox.get_address();
    our_intro.num_changes_estimate = num_changes_estimate;
    our_intro.distribution_counts = std::move(distribution_counts);
    our_intro.distribution_counts_sum = distribution_counts_sum;
    send(parent->mailbox_manager, intro.intro_mailbox, our_intro);
}

/* `item_seq_pre_item_producer_t` is a `backfill_pre_item_producer_t` that reads from a
`backfill_item_seq_t<backfill_pre_item_t>`. It keeps track of its position in the seq by
moving the elements of the seq to an internal seq; it restores the seq to its original
state when it goes out of scope. */
class item_seq_pre_item_producer_t : public store_view_t::backfill_pre_item_producer_t {
public:
    item_seq_pre_item_producer_t(
            backfill_item_seq_t<backfill_pre_item_t> *_pi,
            const key_range_t::right_bound_t &_c) :
        pre_items(_pi),
        temp_buf(_pi->get_beg_hash(), _pi->get_end_hash(), _pi->get_left_key()),
        last_cursor(_c)
    {
        guarantee(pre_items->get_left_key() <= last_cursor);
        guarantee(pre_items->get_right_key() >= last_cursor);
        guarantee(pre_items->empty_of_items()
            || pre_items->front().range.right > last_cursor,
            "We're trying to start iteration after the first pre-item. This probably "
            "indicates that we skipped backfilling part of the key-space.");
    }
    ~item_seq_pre_item_producer_t() {
        temp_buf.concat(std::move(*pre_items));
        *pre_items = std::move(temp_buf);
    }
    continue_bool_t consume_range(
            key_range_t::right_bound_t *cursor_inout,
            const key_range_t::right_bound_t &limit,
            const std::function<void(const backfill_pre_item_t &)> &callback) {
        guarantee(last_cursor == *cursor_inout);
        if (*cursor_inout == pre_items->get_right_key()) {
            return continue_bool_t::ABORT;
        }
        rassert(pre_items->empty_of_items()
            || pre_items->front().range.right >= *cursor_inout);
        while (true) {
            if (pre_items->empty_of_items()) {
                *cursor_inout = std::min(limit, pre_items->get_right_key());
                break;
            }
            if (key_range_t::right_bound_t(pre_items->front().range.left) < limit) {
                callback(pre_items->front());
            } else {
                *cursor_inout = limit;
                break;
            }
            if (pre_items->front().range.right <= limit) {
                pre_items->pop_front_into(&temp_buf);
            } else {
                *cursor_inout = limit;
                break;
            }
        }
        rassert(*cursor_inout <= pre_items->get_right_key());
        rassert(last_cursor < *cursor_inout);
        last_cursor = *cursor_inout;
        return continue_bool_t::CONTINUE;
    }
    bool try_consume_empty_range(const key_range_t &range) {
        guarantee(key_range_t::right_bound_t(range.left) == last_cursor);
        if (pre_items->get_right_key() >= range.right &&
                (pre_items->empty_of_items() ||
                    key_range_t::right_bound_t(pre_items->front().range.left)
                        >= range.right)) {
            last_cursor = range.right;
            return true;
        } else {
            return false;
        }
    }
    void rewind(const key_range_t::right_bound_t &point) {
        guarantee(last_cursor >= point);
        last_cursor = point;
        while (!temp_buf.empty_of_items() && temp_buf.back().range.right > point) {
            temp_buf.pop_back_into(pre_items);
        }
    }
private:
    /* `pre_items` is the sequence we're reading from */
    backfill_item_seq_t<backfill_pre_item_t> *pre_items;

    /* Temporary storage for pre-items that have been consumed */
    backfill_item_seq_t<backfill_pre_item_t> temp_buf;

    /* Our current position in the sequence, used to sanity-check the method calls we're
    getting. */
    key_range_t::right_bound_t last_cursor;
};

/* When the `client_t` receives a begin-session message from the backfillee, it creates a
`session_t`. The `session_t` is responsible for sending items to the backfillee. When the
session is over, the backfillee will send an end-session message to the `client_t`, which
will destroy the `session_t` and then send an ack-end-session message back to the
backfillee. */
class backfiller_t::client_t::session_t {
public:
    session_t(client_t *_parent, const key_range_t::right_bound_t &_threshold) :
        parent(_parent), threshold(_threshold), pulse_when_pre_items_arrive(nullptr)
    {
        guarantee(parent->pre_items.empty_of_items() ||
            key_range_t::right_bound_t(parent->pre_items.front().range.left)
                >= threshold,
            "Every key must be backfilled at least once; it's not OK to start a new "
            "session after the end of the furthest-right previous session");
        coro_t::spawn_sometime(std::bind(&session_t::run, this, drainer.lock()));
    }

    /* Every time the `client_t` receives more pre-items from the backfillee, it calls
    `on_pre_items()` to notify us. */
    void on_pre_items() {
        if (pulse_when_pre_items_arrive != nullptr) {
            pulse_when_pre_items_arrive->pulse_if_not_already_pulsed();
        }
    }

private:
    void run(auto_drainer_t::lock_t keepalive) {
        with_priority_t p(CORO_PRIORITY_BACKFILL_SENDER);
        try {
            while (threshold != parent->full_region.inner.right) {
                /* Wait until there's room in the semaphore for the chunk we're about to
                process.
                We acquire the maximum size that we want to put in this chunk first,
                and then adjust the semaphore acquisition to the actual size of the
                chunk later. */
                new_semaphore_acq_t sem_acq(
                    &parent->item_throttler, parent->intro.config.item_chunk_mem_size);
                wait_interruptible(
                    sem_acq.acquisition_signal(), keepalive.get_drain_signal());

                /* Wait until we have some pre items, or else we won't be able to make
                any progress on the backfill */
                while (parent->pre_items.get_right_key() == threshold) {
                    cond_t cond;
                    assignment_sentry_t<cond_t *> sentry(
                        &pulse_when_pre_items_arrive, &cond);
                    wait_interruptible(&cond, keepalive.get_drain_signal());
                }

                /* Set up a `region_t` describing the range that still needs to be
                backfilled */
                region_t subregion = parent->full_region;
                subregion.inner.left = threshold.key();

                /* Copy items from the store into `chunk` until the total size hits
                `item_chunk_mem_size`; we finish the backfill range; or we run out of
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
                    item_seq_pre_item_producer_t producer(
                        &parent->pre_items, threshold);

                    /* `consumer_t` is responsible for receiving backfill items and
                    the corresponding metainfo from `send_backfill()` and storing them in
                    `chunk` and in `metainfo`. */
                    class consumer_t : public store_view_t::backfill_item_consumer_t {
                    public:
                        consumer_t(
                                backfill_item_seq_t<backfill_item_t> *_chunk,
                                region_map_t<version_t> *_metainfo,
                                const backfill_config_t *config) :
                            chunk(_chunk), metainfo(_metainfo),
                            remaining_mem_size(
                                static_cast<ssize_t>(config->item_chunk_mem_size)) {
                                guarantee(chunk->get_mem_size() == 0);
                            }
                        void on_item(
                                const region_map_t<binary_blob_t> &item_metainfo,
                                backfill_item_t &&item) THROWS_NOTHING {
                            rassert(key_range_t::right_bound_t(item.range.left) >=
                                chunk->get_right_key());
                            rassert(!item.range.is_empty());
                            on_metainfo(item_metainfo, item.range.right);
                            chunk->push_back(std::move(item));
                        }
                        void on_empty_range(
                                const region_map_t<binary_blob_t> &range_metainfo,
                                const key_range_t::right_bound_t &new_threshold)
                                THROWS_NOTHING {
                            rassert(new_threshold >= chunk->get_right_key());
                            on_metainfo(range_metainfo, new_threshold);
                            chunk->push_back_nothing(new_threshold);
                        }
                        continue_bool_t reserve_memory(size_t mem_size) THROWS_NOTHING {
                            remaining_mem_size -= static_cast<ssize_t>(mem_size);
                            if (remaining_mem_size <= 0) {
                                return continue_bool_t::ABORT;
                            } else {
                                return continue_bool_t::CONTINUE;
                            }
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
                            metainfo->extend_keys_right(
                                to_version_map(new_metainfo.mask(mask)));
                        }
                        backfill_item_seq_t<backfill_item_t> *const chunk;
                        region_map_t<version_t> *const metainfo;
                        ssize_t remaining_mem_size;
                    } consumer(&chunk, &metainfo, &parent->intro.config);

                    parent->parent->store->send_backfill(
                        parent->common_version.mask(subregion),
                        &producer, &consumer, keepalive.get_drain_signal());

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
                    equal to `item_chunk_mem_size`, and then transfer the semaphore
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
                            metainfo.map(metainfo.get_domain(),
                                [](const version_t &v) { return v.timestamp; }));

                        /* Discard pre-items we don't need anymore. This has two
                        purposes: it saves memory, and it keeps `pre_items` consistent
                        with `common_version`. However, we note that the domain of the
                        `pre_items` seq still starts at the left-hand end of the range to
                        be backfilled, representing that there are no pre items there.
                        This is correct because after the backfillee applies the item we
                        just sent, it won't have any divergent data relative to us in
                        that region. */
                        size_t old_size = parent->pre_items.get_mem_size();
                        parent->pre_items.delete_to_key(threshold);
                        parent->pre_items.push_front_nothing(
                            key_range_t::right_bound_t(parent->full_region.inner.left));
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
    cond_t *pulse_when_pre_items_arrive;

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

