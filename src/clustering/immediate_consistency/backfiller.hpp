// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_

#include <map>
#include <utility>

#include "clustering/generic/registrar.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "store_view.hpp"

/* `backfiller_t` is responsible for copying the given store's state to other servers via
`backfillee_t`.

It assumes that if the state of the underlying store changes, the only change will be to
apply writes. In particular, it might break if the underlying store receives a backfill
or erases data while the `backfiller_t` exists. */

class backfiller_t : public home_thread_mixin_debug_only_t {
public:
    backfiller_t(mailbox_manager_t *_mailbox_manager,
                 branch_history_manager_t *_branch_history_manager,
                 store_view_t *_store);

    backfiller_bcard_t get_business_card() {
        return backfiller_bcard_t {
            store->get_region(),
            registrar.get_business_card() };
    }

private:
    class client_t {
    public:
        client_t(backfiller_t *, const backfiller_bcard_t::intro_1_t &intro, signal_t *);

    private:
        class session_t;
        void on_begin_session(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token,
            const key_range_t::right_bound_t &threshold);
        void on_end_session(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token);
        void on_ack_atoms(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token,
            size_t mem_size);
        void on_pre_atoms(
            signal_t *interruptor,
            const fifo_enforcer_write_token_t &write_token,
            const backfill_atom_seq_t<backfill_pre_atom_t> &chunk);

        backfiller_t *const parent;
        backfiller_bcard_t::intro_1_t const intro;
        region_t const full_region;

        fifo_enforcer_source_t fifo_source;
        fifo_enforcer_sink_t fifo_sink;

        region_map_t<state_timestamp_t> common_version;
        backfill_atom_seq_t<backfill_pre_atom_t> pre_atoms;

        scoped_ptr_t<session_t> current_session;

        new_semaphore_t atom_throttler;
        new_semaphore_acq_t atom_throttler_acq;

        backfiller_bcard_t::pre_atoms_mailbox_t pre_atoms_mailbox;
        backfiller_bcard_t::go_mailbox_t go_mailbox;
        backfiller_bcard_t::stop_mailbox_t stop_mailbox;
        backfiller_bcard_t::ack_atoms_mailbox_t ack_atoms_mailbox;
    };

    mailbox_manager_t *const mailbox_manager;
    branch_history_manager_t *const branch_history_manager;
    store_view_t *const store;

    registrar_t<backfiller_bcard_t::intro_1_t, backfiller_t *, client_t> registrar;

    DISABLE_COPYING(backfiller_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_ */
