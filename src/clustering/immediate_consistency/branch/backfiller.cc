// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/backfiller.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "btree/parallel_traversal.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/semaphore.hpp"
#include "rpc/semilattice/view.hpp"
#include "stl_utils.hpp"

// The number of backfill chunks that may be sent but not yet acknowledged (~processed)
// by the receiver.
// Each chunk can contain multiple key/value pairs, but its (approximate) maximum
// size is limited by BACKFILL_MAX_KVPAIRS_SIZE as defined in btree/backfill.hpp.
// When setting this value, keep memory consumption in mind.
// Must be >= ALLOCATION_CHUNK in backfillee.cc, or backfilling will stall and
// never finish.
#define MAX_CHUNKS_OUT 64

inline state_timestamp_t get_earliest_timestamp_of_version_range(const version_range_t &vr) {
    return vr.earliest.timestamp;
}

template <class protocol_t>
backfiller_t<protocol_t>::backfiller_t(mailbox_manager_t *mm,
                                       branch_history_manager_t<protocol_t> *bhm,
                                       store_view_t<protocol_t> *_svs)
    : mailbox_manager(mm), branch_history_manager(bhm),
      svs(_svs),
      backfill_mailbox(mailbox_manager,
                       boost::bind(&backfiller_t::on_backfill, this, _1, _2, _3, _4, _5, _6, _7, auto_drainer_t::lock_t(&drainer))),
      cancel_backfill_mailbox(mailbox_manager,
                              boost::bind(&backfiller_t::on_cancel_backfill, this, _1, auto_drainer_t::lock_t(&drainer))),
      request_progress_mailbox(mailbox_manager,
                               boost::bind(&backfiller_t::request_backfill_progress, this, _1, _2, auto_drainer_t::lock_t(&drainer))) { }

template <class protocol_t>
backfiller_business_card_t<protocol_t> backfiller_t<protocol_t>::get_business_card() {
    return backfiller_business_card_t<protocol_t>(backfill_mailbox.get_address(),
                                                  cancel_backfill_mailbox.get_address(),
                                                  request_progress_mailbox.get_address());
}

template <class protocol_t>
bool backfiller_t<protocol_t>::confirm_and_send_metainfo(typename store_view_t<protocol_t>::metainfo_t metainfo,
                                                         region_map_t<protocol_t, version_range_t> start_point,
                                                         mailbox_addr_t<void(region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t>)> end_point_cont) {
    guarantee(metainfo.get_domain() == start_point.get_domain());
    region_map_t<protocol_t, version_range_t> end_point =
        region_map_transform<protocol_t, binary_blob_t, version_range_t>(metainfo,
                                                                         &binary_blob_t::get<version_range_t>);

#ifndef NDEBUG
    // TODO: Should the rassert calls in this block of code be return
    // false statements instead of assertions?  Tim doesn't know.
    // Figure this out.

    /* Confirm that `start_point` is a point in our past */
    typedef region_map_t<protocol_t, version_range_t> version_map_t;

    {
        on_thread_t th(branch_history_manager->home_thread());
        for (typename version_map_t::const_iterator it = start_point.begin();
             it != start_point.end();
             ++it) {
            for (typename version_map_t::const_iterator jt = end_point.begin();
                 jt != end_point.end();
                 ++jt) {
                typename protocol_t::region_t ixn = region_intersection(it->first, jt->first);
                if (!region_is_empty(ixn)) {
                    version_t start = it->second.latest;
                    version_t end = jt->second.earliest;
                    rassert(start.timestamp <= end.timestamp);
                    rassert(version_is_ancestor(branch_history_manager, start, end, ixn));
                }
            }
        }
    }
#endif

    /* Package a subset of the branch history graph that includes every branch
    that `end_point` mentions and all their ancestors recursively */
    branch_history_t<protocol_t> branch_history;
    {
        on_thread_t th(branch_history_manager->home_thread());
        branch_history_manager->export_branch_history(end_point, &branch_history);
    }

    /* Transmit `end_point` to the backfillee */
    send(mailbox_manager, end_point_cont, end_point, branch_history);

    return true;
}

template <class protocol_t>
void do_send_chunk(mailbox_manager_t *mbox_manager,
                   mailbox_addr_t<void(typename protocol_t::backfill_chunk_t, fifo_enforcer_write_token_t)> chunk_addr,
                   const typename protocol_t::backfill_chunk_t &chunk,
                   fifo_enforcer_source_t *fifo_src,
                   co_semaphore_t *chunk_semaphore,
                   signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    chunk_semaphore->co_lock_interruptible(interruptor);
    send(mbox_manager, chunk_addr, chunk, fifo_src->enter_write());
}

template <class protocol_t>
class backfiller_send_backfill_callback_t : public send_backfill_callback_t<protocol_t> {
public:
    backfiller_send_backfill_callback_t(const region_map_t<protocol_t, version_range_t> *start_point,
                                        mailbox_addr_t<void(region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t>)> end_point_cont,
                                        mailbox_manager_t *mailbox_manager,
                                        mailbox_addr_t<void(typename protocol_t::backfill_chunk_t, fifo_enforcer_write_token_t)> chunk_cont,
                                        fifo_enforcer_source_t *fifo_src,
                                        co_semaphore_t *chunk_semaphore,
                                        backfiller_t<protocol_t> *backfiller)
        : start_point_(start_point),
          end_point_cont_(end_point_cont),
          mailbox_manager_(mailbox_manager),
          chunk_cont_(chunk_cont),
          fifo_src_(fifo_src),
          chunk_semaphore_(chunk_semaphore),
          backfiller_(backfiller) { }

    bool should_backfill_impl(const typename store_view_t<protocol_t>::metainfo_t &metainfo) {
        return backfiller_->confirm_and_send_metainfo(metainfo, *start_point_, end_point_cont_);
    }

    void send_chunk(const typename protocol_t::backfill_chunk_t &chunk, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        do_send_chunk<protocol_t>(mailbox_manager_, chunk_cont_, chunk, fifo_src_, chunk_semaphore_, interruptor);
    }
private:
    const region_map_t<protocol_t, version_range_t> *start_point_;
    mailbox_addr_t<void(region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t>)> end_point_cont_;
    mailbox_manager_t *mailbox_manager_;
    mailbox_addr_t<void(typename protocol_t::backfill_chunk_t, fifo_enforcer_write_token_t)> chunk_cont_;
    fifo_enforcer_source_t *fifo_src_;
    co_semaphore_t *chunk_semaphore_;
    backfiller_t<protocol_t> *backfiller_;

    DISABLE_COPYING(backfiller_send_backfill_callback_t);
};

template <class protocol_t>
void backfiller_t<protocol_t>::on_backfill(backfill_session_id_t session_id,
                                           const region_map_t<protocol_t, version_range_t> &start_point,
                                           const branch_history_t<protocol_t> &start_point_associated_branch_history,
                                           mailbox_addr_t<void(region_map_t<protocol_t, version_range_t>, branch_history_t<protocol_t>)> end_point_cont,
                                           mailbox_addr_t<void(typename protocol_t::backfill_chunk_t, fifo_enforcer_write_token_t)> chunk_cont,
                                           mailbox_addr_t<void(fifo_enforcer_write_token_t)> done_cont,
                                           mailbox_addr_t<void(mailbox_addr_t<void(int)>)> allocation_registration_box,
                                           auto_drainer_t::lock_t keepalive) {

    assert_thread();
    guarantee(region_is_superset(svs->get_region(), start_point.get_domain()));

    /* Set up a local interruptor cond and put it in the map so that this
       session can be interrupted if the backfillee decides to abort */
    cond_t local_interruptor;
    map_insertion_sentry_t<backfill_session_id_t, cond_t *> be_interruptible(&local_interruptors, session_id, &local_interruptor);

    /* Set up a local progress monitor so people can query us for progress. */
    traversal_progress_combiner_t local_progress;
    map_insertion_sentry_t<backfill_session_id_t, traversal_progress_combiner_t *> display_progress(&local_backfill_progress, session_id, &local_progress);

    /* Set up a cond that gets pulsed if we're interrupted by either the
       backfillee stopping or the backfiller destructor being called, but don't
       wait on that cond yet. */
    wait_any_t interrupted(&local_interruptor, keepalive.get_drain_signal());

    static_semaphore_t chunk_semaphore(MAX_CHUNKS_OUT);
    mailbox_t<void(int)> receive_allocations_mbox(mailbox_manager, boost::bind(&co_semaphore_t::unlock, &chunk_semaphore, _1));
    send(mailbox_manager, allocation_registration_box, receive_allocations_mbox.get_address());

    try {
        {
            on_thread_t th(branch_history_manager->home_thread());
            branch_history_manager->import_branch_history(start_point_associated_branch_history, keepalive.get_drain_signal());
        }

        // TODO: Describe this fifo source's purpose a bit.  It's for ordering backfill operations, right?
        fifo_enforcer_source_t fifo_src;

        read_token_pair_t send_backfill_token_pair;
        svs->new_read_token_pair(&send_backfill_token_pair);

        backfiller_send_backfill_callback_t<protocol_t>
            send_backfill_cb(&start_point, end_point_cont, mailbox_manager, chunk_cont, &fifo_src, &chunk_semaphore, this);

        /* Actually perform the backfill */
        svs->send_backfill(
                     region_map_transform<protocol_t, version_range_t, state_timestamp_t>(
                                                      start_point,
                                                      &get_earliest_timestamp_of_version_range
                                                      ),
                     &send_backfill_cb,
                     local_backfill_progress[session_id],
                     &send_backfill_token_pair,
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


template <class protocol_t>
void backfiller_t<protocol_t>::on_cancel_backfill(backfill_session_id_t session_id, UNUSED auto_drainer_t::lock_t) {

    assert_thread();

    typename std::map<backfill_session_id_t, cond_t *>::iterator it =
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


template <class protocol_t>
void backfiller_t<protocol_t>::request_backfill_progress(backfill_session_id_t session_id,
                                                         mailbox_addr_t<void(std::pair<int, int>)> response_mbox,
                                                         auto_drainer_t::lock_t) {
    if (std_contains(local_backfill_progress, session_id) && local_backfill_progress[session_id]) {
        progress_completion_fraction_t fraction = local_backfill_progress[session_id]->guess_completion();
        std::pair<int, int> pair_fraction = std::make_pair(fraction.estimate_of_released_nodes, fraction.estimate_of_total_nodes);
        send(mailbox_manager, response_mbox, pair_fraction);
    } else {
        send(mailbox_manager, response_mbox, std::make_pair(-1, -1));
    }

    //TODO indicate an error has occurred
}


#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class backfiller_t<mock::dummy_protocol_t>;
template class backfiller_t<memcached_protocol_t>;
template class backfiller_t<rdb_protocol_t>;
