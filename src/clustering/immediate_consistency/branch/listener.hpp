// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_

#include <map>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/min_version_enforcer.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "concurrency/semaphore.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/types.hpp"
#include "timestamps.hpp"
#include "utils.hpp"

template <class> class std_function_callback_t;
class branch_history_manager_t;
class broadcaster_t;
template <class> class clone_ptr_t;
template <class> class coro_pool_t;
class intro_receiver_t;
template <class> class registrant_t;
class replier_t;
template <class> class semilattice_read_view_t;
template <class> class semilattice_readwrite_view_t;
template <class> class watchable_t;
class backfill_throttler_t;

/* `listener_t` keeps a store-view in sync with a branch. Its constructor
contacts a `broadcaster_t` to sign up for real-time updates, and also backfills
from a `replier_t` to get a copy of all the existing data. As long as the
`listener_t` exists and nothing goes wrong, it will keep in sync with the
branch.

There are four ways a `listener_t` can go wrong:
 *  You can interrupt the constructor. It will throw `interrupted_exc_t`. The
    store may be left in a half-backfilled state; you can determine this through
    the store's metadata.
 *  It can fail to contact the backfiller. In that case,
    the constructor will throw `backfiller_lost_exc_t`.
 *  It can fail to contact the broadcaster. In this case it will throw `broadcaster_lost_exc_t`.
 *  It can successfully join the branch, but then lose contact with the
    broadcaster later. In that case, `get_broadcaster_lost_signal()` will be
    pulsed when it loses touch.
*/

class listener_t {
public:
    class backfiller_lost_exc_t : public std::exception {
    public:
        const char *what() const throw () {
            return "Lost contact with backfiller";
        }
    };

    class broadcaster_lost_exc_t : public std::exception {
    public:
        const char *what() const throw () {
            return "Lost contact with broadcaster";
        }
    };

    listener_t(
            const base_path_t &base_path,
            io_backender_t *io_backender,
            mailbox_manager_t *mm,
            const server_id_t &server_id,
            backfill_throttler_t *backfill_throttler,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > > broadcaster_metadata,
            branch_history_manager_t *branch_history_manager,
            store_view_t *svs,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t> > > > replier,
            perfmon_collection_t *backfill_stats_parent,
            signal_t *interruptor,
            order_source_t *order_source,
            double *backfill_progress_out   /* can be null */)
            THROWS_ONLY(interrupted_exc_t, backfiller_lost_exc_t, broadcaster_lost_exc_t);

    /* This version of the `listener_t` constructor is called when we are
    becoming the first mirror of a new branch. It should only be called once for
    each `broadcaster_t`. */
    listener_t(
            const base_path_t &base_path,
            io_backender_t *io_backender,
            mailbox_manager_t *mm,
            const server_id_t &server_id,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > > broadcaster_metadata,
            branch_history_manager_t *branch_history_manager,
            broadcaster_t *broadcaster,
            perfmon_collection_t *backfill_stats_parent,
            signal_t *interruptor,
            order_source_t *order_source) THROWS_ONLY(interrupted_exc_t);

    ~listener_t();

    /* Returns a signal that is pulsed if the mirror is not in contact with the
    master. */
    signal_t *get_broadcaster_lost_signal();

    // Getters used by the replier :(
    // TODO: Some of these can and should be passed directly to the replier?
    store_view_t *svs() const {
        guarantee(svs_ != NULL);
        return svs_;
    }

    branch_id_t branch_id() const {
        guarantee(!branch_id_.is_nil());
        return branch_id_;
    }

    listener_business_card_t::writeread_mailbox_t::address_t writeread_address() const {
        return writeread_mailbox_.get_address();
    }

    listener_business_card_t::read_mailbox_t::address_t read_address() const {
        return read_mailbox_.get_address();
    }

    void wait_for_version(state_timestamp_t timestamp, signal_t *interruptor);

    const listener_intro_t &registration_done_cond_value() const {
        return registration_done_cond_.wait();
    }

    /* Interface for performing local reads without going through a mailbox */
    read_response_t local_read(
            const read_t &read,
            min_version_token_t min_version_token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    write_response_t local_writeread(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void local_write(
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);


    class write_queue_entry_t {
    public:
        write_queue_entry_t() { }
        write_queue_entry_t(const write_t &w, state_timestamp_t ts, order_token_t _order_token, fifo_enforcer_write_token_t ft) :
            write(w), timestamp(ts), order_token(_order_token), fifo_token(ft) { }
        write_t write;
        state_timestamp_t timestamp;
        order_token_t order_token;
        fifo_enforcer_write_token_t fifo_token;
    };

private:
    // TODO: This boost optional boost optional crap is ... crap.  This isn't Haskell, this is *real* programming, people.
    static boost::optional<boost::optional<backfiller_business_card_t> > get_backfiller_from_replier_bcard(const boost::optional<boost::optional<replier_business_card_t> > &replier_bcard);

    // TODO: Boost boost optional optional business card business card protocol_tee tee tee tee piii kaaa chuuuuu!
    static boost::optional<boost::optional<registrar_business_card_t<listener_business_card_t> > > get_registrar_from_broadcaster_bcard(const boost::optional<boost::optional<broadcaster_business_card_t> > &broadcaster_bcard);

    /* `try_start_receiving_writes()` is called from within the constructors. It
    tries to register with the master. It throws `interrupted_exc_t` if
    `interruptor` is pulsed. Otherwise, it fills `registration_result_cond` with
    a value indicating if the registration succeeded or not, and with the intro
    we got from the broadcaster if it succeeded. */
    void try_start_receiving_writes(
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t> > > > broadcaster,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, broadcaster_lost_exc_t);

    void on_write(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void()> ack_addr)
        THROWS_NOTHING;

    void perform_enqueued_write(const write_queue_entry_t &serialized_write, state_timestamp_t backfill_end_timestamp, signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    /* See the note at the place where `writeread_mailbox` is declared for an
    explanation of why `on_writeread()` and `on_read()` are here. */

    void on_writeread(
            signal_t *interruptor,
            const write_t &write,
            state_timestamp_t timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void(write_response_t)> ack_addr,
            write_durability_t durability)
        THROWS_NOTHING;

    void on_read(
            signal_t *interruptor,
            const read_t &read,
            min_version_token_t min_version_token,
            mailbox_addr_t<void(read_response_t)> ack_addr)
        THROWS_NOTHING;

    void advance_current_timestamp_and_pulse_waiters(state_timestamp_t timestamp);

    mailbox_manager_t *const mailbox_manager_;

    server_id_t const server_id_;

    store_view_t *const svs_;

    branch_id_t branch_id_;

    region_t our_branch_region_;

    /* `upgrade_mailbox` and `broadcaster_begin_timestamp` are valid only if we
    successfully registered with the broadcaster at some point. As a sanity
    check, we put them in a `promise_t`, `registration_done_cond`, that only
    gets pulsed when we successfully register. */
    promise_t<listener_intro_t> registration_done_cond_;

    // This uuid exists solely as a temporary used to be passed to
    // uuid_to_str for perfmon_collection initialization and the
    // backfill queue file name
    const uuid_u uuid_;

    perfmon_collection_t perfmon_collection_;
    perfmon_membership_t perfmon_collection_membership_;

    state_timestamp_t current_timestamp_;
    fifo_enforcer_sink_t store_entrance_sink_;
    /* Enforces that reads see every write they need to see (as decided by
    the broadcaster, currently writes that have been acked back to the user). */
    min_version_enforcer_t read_min_version_enforcer_;

    // Used by the replier_t which needs to be able to tell
    // backfillees how up to date it is.
    std::multimap<state_timestamp_t, cond_t *> synchronize_waiters_;

    disk_backed_queue_wrapper_t<write_queue_entry_t> write_queue_;
    fifo_enforcer_sink_t write_queue_entrance_sink_;
    scoped_ptr_t<std_function_callback_t<write_queue_entry_t> > write_queue_coro_pool_callback_;
    adjustable_semaphore_t write_queue_semaphore_;
    cond_t write_queue_has_drained_;

    /* Destroying `write_queue_coro_pool` will stop any invocations of
    `perform_enqueued_write()`. We mustn't access any member variables defined
    below `write_queue_coro_pool` from within `perform_enqueued_write()`,
    because their destructors might have been called. */
    scoped_ptr_t<coro_pool_t<write_queue_entry_t> > write_queue_coro_pool_;

    auto_drainer_t drainer_;

    listener_business_card_t::write_mailbox_t write_mailbox_;

    /* `writeread_mailbox` and `read_mailbox` live on the `listener_t` even
    though they don't get used until the `replier_t` is constructed. The reason
    `writeread_mailbox` has to live here is so we don't drop writes if the
    `replier_t` is destroyed without doing a warm shutdown but the `listener_t`
    stays alive. The reason `read_mailbox` is here is for consistency, and to
    have all the query-handling code in one place. */
    listener_business_card_t::writeread_mailbox_t writeread_mailbox_;
    listener_business_card_t::read_mailbox_t read_mailbox_;

    /* The local listener registration is released after the registrant_'s destructor
    has unregistered with the broadcaster. That's why we must make sure that
    registrant_ is destructed before local_listener_registration_ is, or we would
    dead-lock on destruction. */
    auto_drainer_t local_listener_registration_;
    scoped_ptr_t<registrant_t<listener_business_card_t> > registrant_;

    DISABLE_COPYING(listener_t);
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(listener_t::write_queue_entry_t);


#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_ */
