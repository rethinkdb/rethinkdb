// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfillee.hpp"

#include "clustering/immediate_consistency/history.hpp"

backfillee_t::backfillee_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store,
        const backfiller_bcard_t &backfiller,
        signal_t *interruptor) :
    mailbox_manager(_mailbox_manager), branch_history_manager(_branch_history_manager),
    store(_store), to_backfill(store->get_region().inner),
    to_send_pre_chyunks(to_backfill)
{
    guarantee(region_is_superset(backfiller.region, store->get_region()));
    guarantee(store->get_region().beg == backfiller.region.beg);
    guarantee(store->get_region().end == backfiller.region.end);

    /* Register with the backfiller and wait until we get the `backfillee_intro_t` */
    cond_t got_intro;
    mailbox_t<void(backfiller_bcard_t::intro_2_t)> intro_mailbox(
        mailbox_manager,
        [&](signal_t *, const backfiller_bcard_t::intro_2_t &i) {
            intro = i;
            got_intro.pulse();
        });
    registrant.init(new registrant_t<backfiller_bcard_t::intro_1_t>(
        mailbox_manager,
        backfiller.registrar,
        backfillee_bcard_t {
            intro_mailbox.get_address()
        }));
    wait_interruptible(&got_intro, interruptor);

    /* Compute the common ancestor of our state and the backfiller's state */
    {
        region_map_t<binary_blob_t> initial_state_blob;
        read_token_t read_token;
        store->new_read_token(&read_token);
        store->do_get_metainfo(order_token_t::ignore.with_read_mode(), &read_token,
            interruptor, &initial_state_blob);
        std::vector<std::pair<region_t, state_timestamp_t> > common_ancestor_parts;
        for (const auto &pair : to_version_map(initial_state_blob)) {
            for (const auto &pair2 : intro.initial_state.mask(pair.first)) {
                for (const auto &pair3 : version_find_common(branch_history_manager,
                        pair.second, pair2.second, pair2.first)) {
                    common_ancestor_parts.push_back(std::make_pair(
                        pair3.first, pair3.second.timestamp));
                }
            }
        }
        common_ancestor = region_map_t<state_timestamp_t>(
            common_ancestor_parts.begin(), common_ancestor_parts.end());
    }
}

bool backfillee_t::go(callback_t *callback, signal_t *interruptor) {
    while (!to_backfill.is_empty()) {
        if (to_send_pre_chunks == to_backfill) {
            send_pre_chunks(interruptor);
        }

        cond_t stop_cond;
        bool callback_returned_false = false;
        
        mailbox_t<void(
                fifo_enforcer_write_token_t,
                std::deque<backfill_chunk_t>,
                region_map_t<version_t>)> chunk_mailbox(
            mailbox_manager,
            std::bind(&backfillee_t::on_chunk, this, ph::_1, ph::_2, ph::_3, ph::_4,
                callback, &stop_cond, &callback_returned_false));

        send(mailbox_manager, intro.start_mailbox,
            fifo_source.enter_write(),
            to_backfill,
            common_ancestor,
            chunk_mailbox.get_address());

        wait_interruptible(&stop_cond, interruptor);

        if (callback_returned_false) {
            send(mailbox_manager, intro.stop_mailbox, fifo_source.enter_write());
            return false;
        }
    }
    return true;
}

void backfillee_t::on_chunk(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        const std::deque<backfill_chunk_t> &data,
        const region_map_t<version_t> &version,
        callback_t *callback,
        new_mutex_t *callback_mutex,
        cond_t *stop_cond,
        bool *callback_returned_false_out) {
    write_token_t write_token;
    object_buffer_t<new_mutex_in_line_t> callback_mutex_in_line;

    {
        fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, fifo_token);
        wait_interruptible(&exit_write, interruptor);
        store->new_write_token(&write_token);
        callback_mutex_in_line.create(callback_mutex);
    }

    store->receive_backfill(
        from_version_map(version),
        data,
        &write_token,
        interruptor);

    wait_interruptible(callback_mutex_in_line->acq_signal(), interruptor);

    if (*callback_returned_false_out) {
        /* Ensure that we don't call `on_progress()` again if it returned `false` */
        return;
    }

    if (callback->on_progress(version)) {
        key_range_t done = version.get_domain().inner;
        guarantee(done.left == to_backfill.left);
        guarantee(done.right <= to_backfill.right);
        guarantee(to_send_pre_chunks.is_empty() ||
            done.right <= key_range_t::right_bound_t(to_send_pre_chunks.left));

        if (done == to_backfill) {
            /* Stop because we finished */
            to_backfill = key_range_t::empty();
            stop_cond->pulse_if_not_already_pulsed();
        } else {
            to_backfill.left = done.right.key;
            if (to_send_pre_chunks == to_backfill) {
                /* Stop because we need to send the next batch of `backfill_pre_chunk_t`s
                */
                stop_cond->pulse_if_not_already_pulsed();
            } else {
                /* Continue */
                send(mailbox_manager, intro.continue_mailbox, fifo_source.enter_write());
            }
        }
    } else {
        /* Stop because the callback said to */
        *callback_returned_false_out = true;
        stop_cond->pulse_if_not_already_pulsed();
    }
}

