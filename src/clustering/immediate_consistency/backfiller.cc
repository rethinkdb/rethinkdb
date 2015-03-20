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
    pre_atom_range {
        full_range.inner.left,
        key_range_t::right_bound_t(full_range.inner.left) },
    current_session(nullptr),
    pre_atoms_mailbox(mailbox_manager,
        std::bind(&client_t::on_pre_atoms, this, ph::_1, ph::_2, ph::_3, ph::_4)),
    go_mailbox(mailbox_manager,
        std::bind(&client_t::on_go, this, ph::_1, ph::_2, ph::_3, ph::_4)),
    stop_mailbox(mailbox_manager,
        std::bind(&client_t::on_stop, this, ph::_1, ph::_2, ph::_3)),
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
        const key_range_t &_session_range) :
    session_id(_session_id), parent(_parent), session_range(_session_range),
    atom_throttler(ATOM_PIPELINE_SIZE)
{
    coro_t::spawn_sometime(std::bind(&session_t::run, this, drainer.lock()));
}

void backfiller_t::client_t::session_t::run(auto_drainer_t::lock_t keepalive) {
    
}

void backfiller_t::client_t::on_pre_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const key_range_t &range,
        const std::deque<backfill_pre_atom_t> &pre_atoms) {
    fifo_enforcer_exit_write_t exit_write(&fifo_enforcer, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(pre_atom_range.right == key_range_t::right_bound_t(range.left));
    pre_atom_range.right = range.right;
    pre_atom_queue.insert(
        pre_atom_queue.end(),
        std::make_move_iterator(pre_atoms.begin()),
        std::make_move_iterator(pre_atoms.end()));

    if (pre_atom_waiter != nullptr) {
        pre_atom_waiter->pulse_if_not_already_pulsed();
    }
}

void backfiller_t::client_t::on_go(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const session_id_t &session_id,
        const key_range_t &range) {
    fifo_enforcer_exit_write_t exit_write(&fifo_enforcer, write_token);
    wait_interruptible(&exit_write, interruptor);

    /* Acquire `go_mutex` in case there's a previous instance of `on_go()` that hasn't
    stopped yet */
    new_mutex_acq_t go_mutex_acq(&go_mutex, interruptor);

    new_semaphore_t atom_throttler(ATOM_PIPELINE_SIZE);

    guarantee(current_session == nullptr);
    session_info_t info;
    info.session_id = session_id;
    current_session = &info;

    /* Release the fifo enforcer while we actually do the backfill */
    exit_write.end();

    try {
        wait_any_t interruptor2(interruptor, &info.stop);

        guarantee(range.left == pre_atom_range.left);
        guarantee(range.right == full_region.inner.right);
        key_range_t range_to_do = range;

        bool done = false;
        while (!done) {
            /* Wait until there's room in the semaphore for the chunk we're about to
            process */
            new_semaphore_acq_t sem_acq(&atom_throttler, ATOM_CHUNK_SIZE);
            wait_interruptible(&sem_acq, &interruptor2);

            /* Copy atoms from the store into `callback_t::atoms` until the total size
            hits `ATOM_CHUNK_SIZE`; we finish the backfill range; or we run out of
            pre-atoms. */
            class callback_t : public store_view_t::backfill_callback_t {
            public:
                callback_t(client_t *_parent) : parent(_parent), size(0) { }
                bool on_atom(backfill_atom_t &&atom) {
                    atoms.push_back(atom);
                    size += atom.size();
                    return size < ATOM_CHUNK_SIZE;
                }
                void on_done_range(const region_map_t<binary_blob_t> &mi) {
                    for (const auto &pair : to_version_map(mi)) {
                        metainfo.push_back(pair);
                    }
                }
                bool next_pre_atom(
                        const key_range_t &range,
                        backfill_pre_atom_t const **next_out) {
                    if (parent->pre_atoms.empty()) {
                        if (range.right <= pre_atom_range.right) {
                            /* We know for sure that there are no more pre atoms in this
                            range */
                            *next_out = nullptr;
                        } else {
                            /* There might be more pre atoms in this range that just
                            haven't arrived yet. Abort `send_backfill()` so we can wait
                            for more pre atoms to arrive. */
                            return false;
                        }
                    } else {
                        if (range.right <= key_range_t::right_bound_t(
                                parent->pre_atoms.front().get_region()->left)) {
                            /* The next pre atom is outside the range, and since the pre
                            atoms always come in order, there can't be any more pre atoms
                            in this range. */
                            *next_out = nullptr;
                        } else {
                            /* The next pre atom is inside the range (but maybe only
                            partially), so return it. */
                            *next_out = &parent->pre_atoms.front();
                            parent->current_session->pre_atoms_consumed.splice(
                                parent->current_session->pre_atoms_consumed.end(),
                                parent->pre_atoms,
                                parent->pre_atoms.begin());
                        }
                    }
                }
                client_t *parent;
                std::deque<backfill_atom_t> atoms;
                std::vector<std::pair<region_t, version_t> > metainfo;
                size_t size;
            } callback(this);
            store->send_backfill(
                common_version.mask(region), pre_atoms_queue, &callback, &interruptor2);

            /* Transfer the semaphore ownership */
            info.atom_throttler_acq.transfer_in(std::move(sem_acq));

            /* Send the chunk over the network */
            region_map_t<version_t> metainfo(
                callback.metainfo.begin(), callback.metainfo.end());
            send(mailbox_manager, intro.atoms_mailbox,
                fifo_source.enter(), session_id, metainfo, callback.atoms);

            /* Update `range_to_do` */
            key_range_t range_done = metainfo.get_domain().inner;
            guarantee(region_is_superset(region.inner, range_done));
            guarantee(range_to_do.left == range_done.left);
            if (range_done.right == range_to_do.right) {
                /* We did the whole range so now we're done. */
                guarantee(pre_atom_queue.empty());
                done = true;
            } else {
                /* We stopped because we hit `ATOM_CHUNK_SIZE` or because we ran out of
                pre-atoms. */
                guarantee(range_done.right < range_to_do.right);
                range_to_do.left = range_done.right.key);
                if (key_range_t::right_bound_t(range_to_do.left)
                        >= pre_atom_range.right) {
                    /* Block until more pre-atoms are available */
                    cond_t cond;
                    guarantee(pre_atom_waiter == nullptr);
                    assignment_sentry_t<cond_t *> sentry(&pre_atom_waiter, &cond);
                    wait_interruptible(&cond, &interruptor2);
                }
            }
        }

        /* Even though we're done, block on the interruptor anyway, so that the
        `session_info_t` stays in scope. */
        interruptor2.wait_lazily_unordered();

    } catch (const interrupted_exc_t &) {
        /* The backfillee sent us a stop message; or the backfillee was destroyed; or the
        backfiller was destroyed. */
    }

    /* If there are any pre-atoms left in `pre_atoms_consumed` that were never acked by
    the backfillee, we need to return them to `pre_atom_queue` because they're still
    needed for the next session. */
    pre_atom_queue.splice(
        pre_atom_queue.begin(),
        info.pre_atoms_consumed);
}

void backfiller_t::client_t::on_stop(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const session_id_t &session_id) {
    fifo_enforcer_exit_write_t exit_write(&fifo_enforcer, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(current_session != nullptr);
    guarantee(current_session->session_id == session_id);
    current_session->stop.pulse();
    current_session = nullptr;
}

void backfiller_t::client_t::on_ack_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const session_id_t &session_id,
        const key_range_t &range,
        size_t atoms_size) {
    fifo_enforcer_exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(current_session != nullptr);
    guarantee(current_session->session_id == session_id);

    /* Release `atom_throttler_acq` so that we'll send more atoms to the backfillee */
    new_semaphore_acq_t *sem_acq = &current_session->atom_throttler_acq;
    guarantee(atoms_size <= sem_acq->count());
    if (atoms_size = sem_acq->count()) {
        /* It's illegal to `set_count(0)`, so we need to do this instead */
        sem_acq->reset();
    } else {
        sem_acq->set_count(sem_acq->count() - atoms_size);
    }

    /* Remove the pre-atoms in `range` from the pre-atom queue, tracking the total size
    of the pre-atoms we remove */
    size_t pre_atoms_size = 0;
    guarantee(range.left == pre_atom_range.left);
    guarantee(range.right <= pre_atom_range.right);
    std::list<backfill_pre_atom_t> *consumed = &current_session->pre_atoms_consumed;
    while (!consumed->empty() && consumed->front().get_region().right <= range.right) {
        pre_atoms_size += consumed->front().size();
        consumed->pop_front();
    }
    pre_atoms_range.left = range.right.key;

    /* Ack the pre-atoms to the backfillee so it will send us more pre-atoms */
    send(mailbox_manager, intro.ack_pre_atoms_mailbox,
        fifo_source.enter_write(), pre_atoms_size);
}

