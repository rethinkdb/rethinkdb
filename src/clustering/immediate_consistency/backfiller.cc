// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/backfiller.hpp"

#include <functional>

#include "btree/parallel_traversal.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/semaphore.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/semilattice/view.hpp"
#include "stl_utils.hpp"
#include "store_view.hpp"

// The number of backfill chunks that may be sent but not yet acknowledged (~processed)
// by the receiver.
// Each chunk can contain multiple key/value pairs, but its (approximate) maximum
// size is limited by BACKFILL_MAX_KVPAIRS_SIZE as defined in btree/backfill.hpp.
// When setting this value, keep memory consumption in mind.
// Must be >= ALLOCATION_CHUNK in backfillee.cc, or backfilling will stall and
// never finish.
#define MAX_CHUNKS_OUT 64

backfiller_t::backfiller_t(mailbox_manager_t *mm,
                           branch_history_manager_t *bhm,
                           store_view_t *_svs)
    : mailbox_manager(mm), branch_history_manager(bhm),
      svs(_svs),
      backfill_mailbox(mailbox_manager,
                       std::bind(&backfiller_t::on_backfill, this, ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8)),
      cancel_backfill_mailbox(mailbox_manager,
                              std::bind(&backfiller_t::on_cancel_backfill, this, ph::_1, ph::_2))
      { }

backfiller_business_card_t backfiller_t::get_business_card() {
    return backfiller_business_card_t(backfill_mailbox.get_address(),
                                                  cancel_backfill_mailbox.get_address());
}

bool backfiller_t::confirm_and_send_metainfo(region_map_t<binary_blob_t> metainfo,
                                             region_map_t<version_t> start_point,
                                             mailbox_addr_t<void(region_map_t<version_t>, branch_history_t)> end_point_cont) {
    guarantee(metainfo.get_domain() == start_point.get_domain());
    region_map_t<version_t> end_point = region_map_transform<binary_blob_t, version_t>(
        metainfo, &binary_blob_t::get<version_t>);

    /* Package a subset of the branch history graph that includes every branch
    that `end_point` mentions and all their ancestors recursively */
    branch_history_t branch_history;
    {
        on_thread_t th(branch_history_manager->home_thread());
        branch_history_manager->export_branch_history(end_point, &branch_history);
    }

    /* Transmit `end_point` to the backfillee */
    send(mailbox_manager, end_point_cont, end_point, branch_history);

    return true;
}

class backfiller_send_backfill_callback_t : public send_backfill_callback_t {
public:
    backfiller_send_backfill_callback_t(
            const region_map_t<version_t> *start_point,
            mailbox_addr_t<void(region_map_t<version_t>, branch_history_t)> end_point_cont,
            mailbox_manager_t *mailbox_manager,
            mailbox_addr_t<void(
                backfill_chunk_t,
                double,
                fifo_enforcer_write_token_t
                )> chunk_cont,
            fifo_enforcer_source_t *fifo_src,
            co_semaphore_t *chunk_semaphore,
            backfiller_t *backfiller)
        : start_point_(start_point),
          end_point_cont_(end_point_cont),
          mailbox_manager_(mailbox_manager),
          chunk_cont_(chunk_cont),
          fifo_src_(fifo_src),
          chunk_semaphore_(chunk_semaphore),
          backfiller_(backfiller) { }

    traversal_progress_combiner_t progress_combiner_;

    bool should_backfill_impl(const region_map_t<binary_blob_t> &metainfo) {
        return backfiller_->confirm_and_send_metainfo(metainfo, *start_point_, end_point_cont_);
    }

    void send_chunk(const backfill_chunk_t &chunk, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_semaphore_->co_lock_interruptible(interruptor);
        progress_completion_fraction_t frac = progress_combiner_.guess_completion();
        send(mailbox_manager_, chunk_cont_,
            chunk,
            frac.invalid()
                ? 0.0   /* `frac.invalid()` is only true at start (I think) */
                : static_cast<double>(frac.estimate_of_released_nodes) /
                    frac.estimate_of_total_nodes,
            fifo_src_->enter_write());
    }
private:
    const region_map_t<version_t> *start_point_;
    mailbox_addr_t<void(region_map_t<version_t>, branch_history_t)> end_point_cont_;
    mailbox_manager_t *mailbox_manager_;
    mailbox_addr_t<void(
        backfill_chunk_t,
        double,
        fifo_enforcer_write_token_t
        )> chunk_cont_;
    fifo_enforcer_source_t *fifo_src_;
    co_semaphore_t *chunk_semaphore_;
    backfiller_t *backfiller_;

    DISABLE_COPYING(backfiller_send_backfill_callback_t);
};

void backfiller_t::on_backfill(
        signal_t *interruptor,
        backfill_session_id_t session_id,
        const region_map_t<version_t> &start_point,
        const branch_history_t &start_point_associated_branch_history,
        mailbox_addr_t<void(region_map_t<version_t>, branch_history_t)> end_point_cont,
        mailbox_addr_t<void(
            backfill_chunk_t,
            double,
            fifo_enforcer_write_token_t
            )> chunk_cont,
        mailbox_addr_t<void(fifo_enforcer_write_token_t)> done_cont,
        mailbox_addr_t<void(mailbox_addr_t<void(int)>)> allocation_registration_box) {

    assert_thread();
    guarantee(region_is_superset(svs->get_region(), start_point.get_domain()));

    /* Set up a local interruptor cond and put it in the map so that this
       session can be interrupted if the backfillee decides to abort */
    cond_t local_interruptor;
    map_insertion_sentry_t<backfill_session_id_t, cond_t *> be_interruptible(&local_interruptors, session_id, &local_interruptor);

    /* Set up a cond that gets pulsed if we're interrupted by either the
       backfillee stopping or the backfiller destructor being called, but don't
       wait on that cond yet. */
    wait_any_t interrupted(&local_interruptor, interruptor);

    static_semaphore_t chunk_semaphore(MAX_CHUNKS_OUT);
    mailbox_t<void(int)> receive_allocations_mbox(mailbox_manager,
        [&](signal_t *, int allocs) {
            chunk_semaphore.unlock(allocs);
        });
    send(mailbox_manager, allocation_registration_box, receive_allocations_mbox.get_address());

    try {
        {
            cross_thread_signal_t interruptor_on_bhm_thread(
                &interrupted, branch_history_manager->home_thread());
            on_thread_t th(branch_history_manager->home_thread());
            branch_history_manager->import_branch_history(
                start_point_associated_branch_history,
                &interruptor_on_bhm_thread);
        }

        // TODO: Describe this fifo source's purpose a bit.  It's for ordering backfill operations, right?
        fifo_enforcer_source_t fifo_src;

        read_token_t send_backfill_token;
        svs->new_read_token(&send_backfill_token);

        backfiller_send_backfill_callback_t send_backfill_cb(
            &start_point, end_point_cont, mailbox_manager, chunk_cont, &fifo_src,
            &chunk_semaphore, this);

        /* Actually perform the backfill */
        svs->send_backfill(
                     region_map_transform<version_t, state_timestamp_t>(
                         start_point,
                         [](const version_t &v) { return v.timestamp; } ),
                     &send_backfill_cb,
                     &send_backfill_cb.progress_combiner_,
                     &send_backfill_token,
                     &interrupted);

        /* Send a confirmation */
        send(mailbox_manager, done_cont, fifo_src.enter_write());

    } catch (const interrupted_exc_t &) {
        /* Ignore. If we were interrupted by the backfillee, then it already
           knows the backfill is cancelled. If we were interrupted by the
           backfiller shutting down, the backfillee will find out via the
           directory. */
    }
}


void backfiller_t::on_cancel_backfill(
        UNUSED signal_t *interruptor, backfill_session_id_t session_id) {

    assert_thread();

    std::map<backfill_session_id_t, cond_t *>::iterator it =
        local_interruptors.find(session_id);
    if (it != local_interruptors.end()) {
        it->second->pulse();
    } else {
        /* The backfill ended on its own right as we were trying to cancel
           it. Since the backfill was over, we removed the local interruptor
           from the map, but the cancel message was already in flight. Since
           there is no backfill to cancel, we just ignore the cancel message.
        */
    }
}

