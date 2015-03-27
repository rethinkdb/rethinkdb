// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfillee.hpp"

#include "clustering/immediate_consistency/history.hpp"

/* `PRE_ATOM_PIPELINE_SIZE` is the maximum combined size of the pre-atoms that we send to
the backfiller that it hasn't consumed yet. So the other server may use up to this size
for its pre-atom queue. `PRE_ATOM_CHUNK_SIZE` is the typical size of a pre-atom message
we send over the network. */
static const size_t PRE_ATOM_PIPELINE_SIZE = 4 * MEGABYTE;
static const size_t PRE_ATOM_CHUNK_SIZE = 100 * KILOBYTE;

backfillee_t::backfillee_t(
        mailbox_manager_t *_mailbox_manager,
        branch_history_manager_t *_branch_history_manager,
        store_view_t *_store,
        const backfiller_bcard_t &backfiller,
        signal_t *interruptor) :
    mailbox_manager(_mailbox_manager),
    branch_history_manager(_branch_history_manager),
    store(_store),
    completed_threshold(store->get_region().inner.left),
    pre_atom_throttler(PRE_ATOM_PIPELINE_SIZE),
    ack_pre_atoms_mailbox(mailbox_manager,
        std::bind(&backfillee_t::on_ack_pre_atoms, this, ph::_1, ph::_2, ph::_3)),
    atoms_mailbox(mailbox_manager,
        std::bind(&backfillee_t::on_atoms, this, ph::_1, ph::_2, ph::_3, ph::_4, ph::_5))
{
    guarantee(region_is_superset(backfiller.region, store->get_region()));
    guarantee(store->get_region().beg == backfiller.region.beg);
    guarantee(store->get_region().end == backfiller.region.end);

    backfiller_bcard_t::intro_1_t our_intro;
    our_intro.ack_pre_atoms_mailbox = ack_pre_atoms_mailbox.get_address();
    our_intro.atoms_mailbox = atoms_mailbox.get_address();

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

    /* Spawn the coroutine that will stream pre-atoms to the backfiller. */
    coro_t::spawn_sometime(
        std::bind(&backfillee_t::send_pre_atoms, this, drainer.lock()));
}

void backfillee_t::go(callback_t *callback, signal_t *interruptor) {
    guarantee(current_session == nullptr);
    session_info_t info;
    info.callback = callback;
    info.callback_returned_false = false;
    info.session_id = generate_uuid();
    assignment_sentry_t<session_info_t *> sentry(&current_session, &info);

    send(mailbox_manager, intro.go_mailbox,
        fifo_source.enter_write(), info.session_id);

    wait_interruptible(&info.stop, interruptor);

    send(mailbox_manager, intro.stop_mailbox,
        fifo_source.enter_write(), info.session_id);
}

void backfillee_t::on_ack_pre_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        size_t pre_atoms_size) {
    /* Ordering of these messages is actually irrelevant; we just pass through the fifo
    sink for consistency */
    fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, fifo_token);
    wait_interruptible(&exit_write, interruptor);

    /* Shrink `pre_atom_throttler_acq` so that `send_pre_atoms()` can acquire the
    semaphore again. */
    guarantee(static_cast<int64_t>(pre_atoms_size) <= pre_atom_throttler_acq.count());
    if (static_cast<int64_t>(pre_atoms_size) == pre_atom_throttler_acq.count()) {
        /* It's illegal to `change_count(0)`, so we need to do this instead */
        pre_atom_throttler_acq.reset();
    } else {
        pre_atom_throttler_acq.change_count(
            pre_atom_throttler_acq.count() - pre_atoms_size);
    }
}

void backfillee_t::on_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &fifo_token,
        const backfiller_bcard_t::session_id_t &session_id,
        const region_map_t<version_t> &version,
        const backfill_atom_seq_t<backfill_atom_t> &chunk) {
    rassert(version.get_domain() == chunk.get_region());

    /* Find the session that this chunk is a part of */
    session_info_t *session = current_session;
    if (session == nullptr || session->session_id != session_id) {
        /* The session was aborted */
        return;
    }
    auto_drainer_t::lock_t keepalive = session->drainer.lock();

    /* Get into line for the mutex */
    write_token_t write_token;
    object_buffer_t<new_mutex_in_line_t> mutex_in_line;
    {
        fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, fifo_token);
        wait_interruptible(&exit_write, interruptor);
        store->new_write_token(&write_token);
        mutex_in_line.create(&session->mutex);
    }

    /* Actually record the data in the underlying store */
    class callback_t : public store_view_t::receive_backfill_callback_t {
    public:
        callback_t(backfill_atom_seq_t<backfill_atom_t> const *a) :
            atoms(a), it(atoms->begin()) { }
        void next_atom(backfill_atom_t const **next_out) {
            *next_out = (it != atoms->end()) ? &*it : nullptr;
        }
        void release_atom() {
            ++it;
        }
        backfill_atom_seq_t<backfill_atom_t> const *atoms;
        std::list<backfill_atom_t>::const_iterator it;
    } callback(&chunk);
    store->receive_backfill(
        from_version_map(version),
        &callback,
        &write_token,
        interruptor);

    /* Wait until it's our turn to access the session callback. This ordering is
    important because `callback->on_progress()` must be called in order from left to
    right. */
    wait_interruptible(mutex_in_line->acq_signal(), interruptor);

    if (session->stop.is_pulsed()) {
        /* Ensure that we don't call `on_progress()` again if it returned `STOP_*` */
        return;
    }

    guarantee(completed_threshold == chunk.get_left_key());

    store_view_t::backfill_continue_t cont = session->callback->on_progress(version);

    if (cont == store_view_t::backfill_continue_t::CONTINUE ||
            cont == store_view_t::backfill_continue_t::STOP_AFTER) {
        /* Tell the backfiller that the chunk was accepted. This has two purposes: it
        lets the backfiller forget the pre-atoms for that range, and it tells the
        backfiller that it's OK to send more atoms. */
        send(mailbox_manager, intro.ack_atoms_mailbox,
            fifo_source.enter_write(), session_id, chunk.get_right_key(),
            chunk.get_mem_size());
        completed_threshold = chunk.get_right_key();
    }

    if (cont == store_view_t::backfill_continue_t::STOP_AFTER ||
            cont == store_view_t::backfill_t::STOP_BEFORE ||
            completed_threshold == store->get_region().inner.right) {
        /* We finished the backfill or we were interrupted. */
        session->stop.pulse();
    }
}

void backfillee_t::send_pre_atoms(auto_drainer_t::lock_t keepalive) {
    try {
        key_range_t::right_bound_t pre_atom_sent_threshold(
            store->get_region().inner.left);
        while (pre_atom_send_threshold != store>get_region().inner.right) {
            /* Wait until there's room in the semaphore for the chunk we're about to
            process */
            new_semaphore_acq_t sem_acq(&pre_atom_throttler, PRE_ATOM_CHUNK_SIZE);
            wait_interruptible(&sem_acq, keepalive.get_drain_signal());

            /* Set up a `region_t` describing the range that still needs to be
            backfilled */
            region_t subregion = store->get_region();
            subregion.inner.left = progress.key;

            /* Copy pre-atoms from the store into `callback_t::atoms` until the total
            size hits `PRE_ATOM_CHUNK_SIZE` or we finish the range */
            class callback_t : public store_view_t::send_backfill_pre_callback_t {
            public:
                bool on_pre_atom(backfill_pre_atom_t &&atom) {
                    chunks.push_back(std::move_atom);
                    if (chunk.get_mem_size() < PRE_ATOM_CHUNK_SIZE) {
                        return store_view_t::backfill_continue_t::CONTINUE;
                    } else {
                        return store_view_t::backfill_continue_t::STOP_AFTER;
                    }
                }
                void on_done_range(const key_range_t::right_bound_t &threshold) {
                    chunk.push_back_nothing(threshold);
                }
                backfill_atom_seq_t<backfill_pre_atom_t> chunk;
            } callback { store->get_region().beg, store->get_region().end, progress };

            store->send_backfill_pre(intro.common_version.mask(subregion), &callback,
                keepalive.get_drain_signal());

            /* Adjust for the fact that `chunk.get_mem_size()` isn't precisely equal to
            `PRE_ATOM_CHUNK_SIZE`, and then transfer the semaphore ownership. */
            sem_acq.set_count(callback.chunk.get_mem_size());
            pre_atom_throttler_acq.transfer_in(std::move(sem_acq));

            /* Send the chunk over the network */
            send(mailbox_manager, intro.pre_atoms_mailbox,
                fifo_source.enter(), callback.chunk);

            /* Update `progress` */
            guarantee(callback.chunk.get_left_key() == pre_atom_sent_threshold);
            pre_atom_sent_threshold = callback.chunk.get_right_key();
        }
    } catch (const interrupted_exc_t &) {
        /* The `backfillee_t` was deleted; the backfill is being aborted. */
    }
}

