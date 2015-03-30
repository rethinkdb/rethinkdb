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
    pre_atoms(full_region.beg, full_region.end,
        key_range_t::right_bound_t(full_region.inner.left)),
    atom_throttler(ATOM_PIPELINE_SIZE),
    atom_throttler_acq(&atom_throttler, 0),
    pre_atoms_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_pre_atoms, this, ph::_1, ph::_2, ph::_3)),
    begin_session_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_begin_session, this, ph::_1, ph::_2, ph::_3)),
    end_session_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_end_session, this, ph::_1, ph::_2)),
    ack_atoms_mailbox(parent->mailbox_manager,
        std::bind(&client_t::on_ack_atoms, this, ph::_1, ph::_2, ph::_3))
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

    backfiller_bcard_t::intro_2_t our_intro;
    our_intro.common_version = common_version;
    our_intro.pre_atoms_mailbox = pre_atoms_mailbox.get_address();
    our_intro.begin_session_mailbox = begin_session_mailbox.get_address();
    our_intro.end_session_mailbox = end_session_mailbox.get_address();
    our_intro.ack_atoms_mailbox = ack_atoms_mailbox.get_address();
    send(parent->mailbox_manager, intro.intro_mailbox, our_intro);
}

class backfiller_t::client_t::session_t {
    session_t(client_t *_parent, const key_range_t::right_bound_t &_threshold) :
        parent(_parent), threshold(_threshold)
    {
        coro_t::spawn_sometime(std::bind(&session_t::run, this, drainer.lock()));
    }

    void on_pre_atoms() {
        if (pulse_when_pre_atoms_arrive.has()) {
            pulse_when_pre_atoms_arrive->pulse_if_not_already_pulsed();
        }
    }

private:
    void run(auto_drainer_t::lock_t keepalive) {
        try {
            while (threshold != parent->full_region.inner.right) {
                /* Wait until there's room in the semaphore for the chunk we're about to
                process */
                new_semaphore_acq_t sem_acq(&atom_throttler, ATOM_CHUNK_SIZE);
                wait_interruptible(&sem_acq, keepalive.get_drain_signal());

                /* Set up a `region_t` describing the range that still needs to be
                backfilled */
                region_t subregion = parent->parent->full_region;
                subregion.inner.left = threshold.key;

                /* Copy atoms from the store into `callback_t::atoms` until the total size
                hits `ATOM_CHUNK_SIZE`; we finish the backfill range; or we run out of
                pre-atoms. */

                backfill_atom_seq_t<backfill_atom_t> chunk(
                    parent->full_region.beg, parent->full_region.end,
                    threshold);
                region_map_t<version_t> metainfo;

                {
                    class producer_t :
                        public store_view_t::backfill_pre_atom_producer_t {
                    public:
                        producer_t(
                                backfill_atom_seq_t<backfill_pre_atom_t> *_pre_atoms,
                                scoped_ptr_t<cond_t> *_pulse_when_pre_atoms_arrive) :
                            pre_atoms(_pre_atoms),
                            temp_buf(pre_atoms.get_hash_beg(), pre_atoms.get_hash_end(),
                                pre_atoms.get_left_key()),
                            pulse_when_pre_atoms_arrive(_pulse_when_pre_atoms_arrive)
                            { }
                        ~producer_t() {
                            temp_buf.concat(std::move(*pre_atoms));
                            pre_atoms = std::move(temp_buf);
                        }
                        bool next_pre_atom(
                                const key_range_t::right_bound_t &horizon,
                                backfill_pre_atom_t const **next_out) {
                            if (pre_atoms->first_before_threshold(horizon, next_out)) {
                                return true;
                            } else {
                                pulse_when_pre_atoms_arrive->init(new cond_t);
                                return false;
                            }
                        }
                        void release_pre_atom() {
                            pre_atoms->pop_front_into(temp);
                        }
                    private:
                        backfill_atom_seq_t<backfill_pre_atom_t> *pre_atoms, temp_buf;
                        scoped_ptr_t<cond_t> *pulse_when_pre_atoms_arrive;
                    } producer(&parent->pre_atoms, &pulse_when_pre_atoms_arrive);

                    class consumer_t : public store_view_t::backfill_atom_consumer_t {
                    public:
                        bool on_atom(
                                const region_map_t<binary_blob_t> &atom_metainfo,
                                backfill_atom_t &&atom) {
                            region_t mask = atom.get_region();
                            mask.inner.left = chunk->get_left_key().key;
                            metainfo->concat(atom_metainfo.mask(mask));
                            chunk->push_back(std::move(atom));
                            return (chunk->get_mem_size() < ATOM_CHUNK_SIZE);
                        }
                        void on_empty_range(
                                const region_map_t<binary_blob_t> &range_metainfo,
                                const key_range_t::right_bound_t &new_threshold) {
                            region_t mask;
                            mask.beg = chunk.get_hash_beg();
                            mask.end = chunk.get_hash_end();
                            mask.inner.left = chunk.get_left_key().key;
                            mask.inner.right = new_threshold;
                            metainfo->concat(range_metainfo.mask(mask));
                            chunk.push_back_nothing(new_threshold);
                        }
                        backfill_atom_seq_t<backfill_atom_t> *chunk;
                        region_map_t<version_t> *metainfo;
                    } consumer { &chunk, &metainfo };

                    store->send_backfill(
                        common_version.mask(subregion), &producer, &consumer,
                        keepalive.get_drain_signal());

                    /* `producer` goes out of scope here, so it restores `pre_atoms` to
                    what it was before */
                }

                /* Adjust for the fact that `chunk.get_mem_size()` isn't precisely equal
                to `ATOM_CHUNK_SIZE`, and then transfer the semaphore ownership. */
                sem_acq.change_count(chunk.get_mem_size());
                info.atom_throttler_acq.transfer_in(std::move(sem_acq));

                /* Update `threshold` */
                guarantee(callback.chunk.get_left_key() == threshold);
                threshold = callback.chunk.get_right_key();

                /* Note: It's essential that we update `common_version` and `pre_atoms_*`
                if and only if we send the chunk over the network. So we shouldn't e.g.
                check the interruptor in between. */
                try {
                    /* Send the chunk over the network */
                    send(mailbox_manager, intro.atoms_mailbox,
                        fifo_source.enter(), metainfo, chunk);

                    /* Update `common_version` to reflect the changes that will happen on
                    the backfillee in response to the chunk */
                    common_version.update(metainfo);

                    /* Discard pre-atoms we don't need anymore */
                    size_t old_size = parent->pre_atoms_future.get_mem_size();
                    parent->pre_atoms_future.delete_to_key(threshold);
                    size_t new_size = parent->pre_atoms_future.get_mem_size();

                    /* Notify the backfiller that it's OK to send us more pre atoms */
                    send(mailbox_manager, intro.ack_pre_atoms_mailbox,
                        fifo_source.enter(), old_size - new_size);
                } catch (const interrupted_exc_t &) {
                    crash("We shouldn't be interrupted during this block");
                }

                if (pulse_when_pre_atoms_arrive.has()) {
                    /* The reason we stopped this chunk was because we ran out of pre
                    atoms. Block until more pre atoms are available. */
                    wait_interruptible(pulse_when_pre_atoms_arrive.get(),
                        keepalive.get_drain_signal());
                    pulse_when_pre_atoms_arrive.reset();
                }
            }
        } catch (const interrupted_exc_t &) {
            /* The backfillee sent us a stop message; or the backfillee was destroyed; or
            the backfiller was destroyed. */
        }
    }

    client_t *parent;
    key_range_t::right_bound_t threshold;
    scoped_ptr_t<cond_t> pulse_when_pre_atoms_arrive;
    auto_drainer_t drainer;
};

void backfiller_t::client_t::on_begin_session(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const key_range_t::right_bound_t &threshold) {
    fifo_enforcer_exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(threshold <= pre_atoms.get_left_key(), "Every key must be backfilled at "
        "least once");
    current_session.init(new session_t(this, threshold));
}

void backfiller_t::client_t::on_end_session(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token) {
    fifo_enforcer_exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(current_session.has());
    current_session.reset();
    send(mailbox_manager, intro.ack_end_session_mailbox,
        fifo_source.enter_write());
}

void backfiller_t::client_t::on_ack_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        size_t mem_size) {
    fifo_enforcer_exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    guarantee(mem_size <= atom_throttler_acq.get_count());
    atom_throttler_acq.change_count(atom_throttler_acq.get_count() - mem_size);
}

void backfiller_t::client_t::on_pre_atoms(
        signal_t *interruptor,
        const fifo_enforcer_write_token_t &write_token,
        const backfill_atom_seq_t<backfill_pre_atom_t> &chunk) {
    fifo_enforcer_exit_write_t exit_write(&fifo_sink, write_token);
    wait_interruptible(&exit_write, interruptor);

    pre_atoms.concat(chunk);
    if (current_session.has()) {
        current_session->on_pre_atoms();
    }
}

