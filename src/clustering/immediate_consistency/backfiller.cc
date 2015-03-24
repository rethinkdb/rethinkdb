// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfiller.hpp"

#include "clustering/immediate_consistency/history.hpp"
#include "rdb_protocol/protocol.hpp"
#include "store_view.hpp"

/* `ATOM_PIPELINE_SIZE` is the maximum combined size of the atoms that we send to the
backfillee that it hasn't consumed yet. `ATOM_CHUNK_SIZE` is the typical size of an atom
message we send over the network. */
static const size_t ATOM_PIPELINE_SIZE = 4 * MEGABYTE;
static const size_t ATOM_CHUNK_SIZE = 100 * KILOBYTE;

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
    pre_atoms_past(full_region.beg, full_region.end,
        key_range_t::right_bound_t(full_region.left)),
    pre_atoms_future(full_region.beg, full_region.end,
        key_range_t::right_bound_t(full_region.left)),
    acked_threshold(full_region.left),
    pre_atoms_mailbox(mailbox_manager,
        std::bind(&client_t::on_pre_atoms, this, ph::_1, ph::_2, ph::_3)),
    go_mailbox(mailbox_manager,
        std::bind(&client_t::on_go, this, ph::_1, ph::_2, ph::_3, ph::_4)),
    stop_mailbox(mailbox_manager,
        std::bind(&client_t::on_stop, this, ph::_1, ph::_2, ph::_3, ph::_4)),
    ack_atoms_mailbox(mailbox_manager,
        std::bind(&client_t::on_ack_atoms, this, ph::_1, ph::_2, ph::_3, ph::_4, ph::_5))
{
    /* Compute the common ancestor of our version and the backfillee's version */
    {
        region_map_t<binary_blob_t> our_version_blob;
        read_token_t read_token;
        parent->store->new_read_token(&read_token);
        parent->store->do_get_metainfo(order_token_t::ignore.with_read_mode(),
            &read_token, interruptor, &initial_state_blob);
        region_map_t<version_t> our_version = to_version_map(our_version_blob);

        branch_history_combiner_t combined_history(
            parent->branch_history_manager,
            &intro.initial_version_history);

        std::vector<std::pair<region_t, state_timestamp_t> > common_pairs;
        for (const auto &pair1 : our_version) {
            for (const auto &pair2 : intro.initial_version.mask(pair1.region)) {
                for (const auto &pair3 : version_find_common(&combined_history,
                        pair1.second, pair2.second, pair2.first)) {
                    common_pairs.push_back(pair3.first, pair3.second.timestamp);
                }
            }
        }
        common_version = region_map_t<state_timestamp_t>(common_pairs);
    }

    backfiller_bcard_t::intro_2_t our_intro;
    our_intro.common_version = common_version;
    our_intro.pre_atoms_mailbox = pre_atoms_mailbox.get_address();
    our_intro.go_mailbox = go_mailbox.get_address();
    our_intro.stop_mailbox = stop_mailbox.get_address();
    our_intro.ack_atoms_mailbox = ack_atoms_mailbox.get_address();
    send(mailbox_manager, intro.intro_mailbox, our_intro);
}

backfiller_t::client_t::session_t::session_t(
        client_t *_parent,
        const backfiller_bcard_t::session_id_t &_session_id,
        const key_range_t::right_bound_t &_start_key) :
    session_id(_session_id), parent(_parent), threshold(_start_key),
    atom_throttler(ATOM_PIPELINE_SIZE)
{
    coro_t::spawn_sometime(std::bind(&session_t::run, this, drainer.lock()));
}

void backfiller_t::client_t::session_t::on_ack_atoms(size_t mem_size) {
    /* Shrink `atom_throttler_acq` so that `run()` can acquire the semaphore again. */
    guarantee(mem_size <= atom_throttler_acq.count());
    if (mem_size == atom_throttler_acq.count()) {
        /* It's illegal to `set_count(0)`, so we need to do this instead */
        atom_throttler_acq.reset();
    } else {
        atom_throttler_acq.set_count(atom_throttler_acq.count() - mem_size);
    }
}

void backfiller_t::client_t::session_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        while (threshold != parent->full_region.inner.right) {
            /* Wait until there's room in the semaphore for the chunk we're about to
            process */
            new_semaphore_acq_t sem_acq(&atom_throttler, ATOM_CHUNK_SIZE);
            wait_interruptible(&sem_acq, keepalive.get_drain_signal());

            /* Set up a `region_t` describing the range that still needs to be backfilled
            */
            region_t subregion = parent->parent->full_region;
            subregion.inner.left = threshold.key;

            /* Copy atoms from the store into `callback_t::atoms` until the total size
            hits `ATOM_CHUNK_SIZE`; we finish the backfill range; or we run out of
            pre-atoms. */
            class callback_t : public store_view_t::send_backfill_callback_t {
            public:
                callback_t(session_t *_parent) : parent(_parent) { }
                bool on_atom(backfill_atom_t &&atom) {
                    if (chunk.get_mem_size() < ATOM_CHUNK_SIZE) {
                        chunk.push_back(std::move(atom));
                        return true;
                    } else {
                        return false;
                    }
                }
                void on_done_range(const region_map_t<binary_blob_t> &mi) {
                    for (const auto &pair : to_version_map(mi)) {
                        metainfo.push_back(pair);
                    }
                    chunk.push_back_nothing(mi.get_region().inner.right);
                }
                bool next_pre_atom(
                        const key_range_t::right_bound_t &threshold,
                        backfill_pre_atom_t const **next_out) {
                    backfiller_t::client_t *gparent = parent->parent;
                    if (gparent->pre_atoms_future.empty() &&
                            gparent->pre_atoms_future.get_right_key() < threshold) {
                        /* We don't have enough information to determine if there's
                        another pre-atom in this range or not, so we have to abort and
                        wait for more pre-atoms */
                        parent->pulse_when_pre_atoms_arrive.init(new cond_t);
                        return false;
                    } else {
                        if (gparent->pre_atoms_future.empty()
                                || key_range_t::right_bound_t(gparent->pre_atoms_future
                                        .front().get_region().left) >= threshold) {
                            *next_out = nullptr;
                        } else {
                            *next_out = &gparent->pre_atoms_future.front();
                        }
                        return true;
                    }
                }
                void release_pre_atom() {
                    parent->parent->pre_atoms_future.pop_front_into(
                        &parent->parent->pre_atoms_past);
                }
                session_t *parent;
                backfill_atom_seq_t<backfill_atom_t> chunk;
                std::vector<std::pair<region_t, version_t> > metainfo;
            } callback(this);
            store->send_backfill(common_version.mask(subregion), &callback,
                keepalive.get_drain_signal());

            /* Adjust for the fact that `chunk.get_mem_size()` isn't precisely equal to
            `ATOM_CHUNK_SIZE`, and then transfer the semaphore ownership. */
            sem_acq.set_count(callback.chunk.get_mem_size());
            info.atom_throttler_acq.transfer_in(std::move(sem_acq));

            /* Send the chunk over the network */
            region_map_t<version_t> metainfo(
                callback.metainfo.begin(), callback.metainfo.end());
            send(mailbox_manager, intro.atoms_mailbox,
                fifo_source.enter(), session_id, metainfo, callback.chunk);

            /* Update `threshold` */
            guarantee(callback.chunk.get_left_key() == threshold);
            threshold = callback.chunk.get_right_key();

            if (pulse_when_pre_atoms_arrive.has()) {
                /* The reason we stopped this chunk was because we ran out of pre atoms.
                Block until more pre atoms are available. */
                wait_interruptible(pulse_when_pre_atoms_arrive.get(),
                    keepalive.get_drain_signal());
                pulse_when_pre_atoms_arrive.reset();
            }
        }
    } catch (const interrupted_exc_t &) {
        /* The backfillee sent us a stop message; or the backfillee was destroyed; or the
        backfiller was destroyed. */
    }

    /* If there are any pre-atoms left in `pre_atoms_consumed` that were never acked by
    the backfillee, we need to return them to `parent->pre_atoms` because they're still
    needed for the next session. */
    parent->pre_atoms_past.concat(std::move(parent->pre_atoms_future));
    parent->pre_atoms_future = std::move(parent->pre_atoms_past);
    parent->pre_atoms_past = backfill_atom_seq_t(
        parent->full_region.beg, parent->full_region.end, parent->acked_threshold);
}

void backfiller_t::client_t::on_pre_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const backfill_atom_seq_t<backfill_pre_atom_t> &chunk) {
    fifo_enforcer_exit_write_t exit_write(&fifo_enforcer, write_token);
    wait_interruptible(&exit_write, interruptor);
    pre_atoms.concat(chunk);
    if (current_session.has() && current_session->pulse_when_pre_atoms_arrive.has()) {
        current_session->pulse_when_pre_atoms_arrive->pulse_if_not_already_pulsed();
    }
}

void backfiller_t::client_t::on_go(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const session_id_t &session_id) {
    fifo_enforcer_exit_write_t exit_write(&fifo_enforcer, write_token);
    wait_interruptible(&exit_write, interruptor);
    current_session.init(new session_t(this, session_id));
}

void backfiller_t::client_t::on_stop(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const session_id_t &session_id) {
    fifo_enforcer_exit_write_t exit_write(&fifo_enforcer, write_token);
    wait_interruptible(&exit_write, interruptor);
    guarantee(current_session.has());
    guarantee(current_session->session_id == session_id);
    current_session.reset();
}

void backfiller_t::client_t::on_ack_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const session_id_t &session_id,
        const key_range_t::right_bound_t &new_threshold,
        size_t atoms_size) {
    fifo_enforcer_exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    threshold = new_threshold;

    /* Tell the session that it's OK to send more atoms */
    guarantee(current_session.has());
    guarantee(current_session->session_id == session_id);
    current_session->on_ack_atoms(atoms_size);

    /* Remove the pre-atoms up to the new `threshold` from the pre-atom queue */
    size_t old_size = pre_atoms_past.get_mem_size();
    pre_atoms_past.delete_to_key(new_threshold);
    size_t deleted_size = old_size - pre_atoms_past.get_mem_size();

    /* Ack the pre-atoms to the backfillee so it will send us more pre-atoms */
    send(mailbox_manager, intro.ack_pre_atoms_mailbox,
        fifo_source.enter_write(), deleted_size);
}

