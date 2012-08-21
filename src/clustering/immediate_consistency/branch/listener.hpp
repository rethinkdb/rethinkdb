#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_

#include <map>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "concurrency/semaphore.hpp"
#include "serializer/types.hpp"
#include "timestamps.hpp"
#include "utils.hpp"

template <class T> class branch_history_manager_t;
template <class T> class broadcaster_t;
template <class T> class intro_receiver_t;
template <class T> class registrant_t;
template <class T> class replier_t;
template <class T> class semilattice_read_view_t;
template <class T> class semilattice_readwrite_view_t;

/* `listener_t` keeps a store-view in sync with a branch. Its constructor
backfills from an existing mirror on a branch into the store, and as long as it
exists the store will receive real-time updates.

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

template <class protocol_t>
class listener_intro_t {
public:
    typename listener_business_card_t<protocol_t>::upgrade_mailbox_t::address_t upgrade_mailbox;
    typename listener_business_card_t<protocol_t>::downgrade_mailbox_t::address_t downgrade_mailbox;
    state_timestamp_t broadcaster_begin_timestamp;
};


template<class protocol_t>
class listener_t {
public:
    /* If the number of writes the broadcaster has sent us minus the number of
    responses it's gotten is greater than this, then it is not allowed to send
    another write until it gets another response. */
    static const int MAX_OUTSTANDING_WRITES_FROM_BROADCASTER = 1000;

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
            io_backender_t *io_backender,
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
            branch_history_manager_t<protocol_t> *branch_history_manager,
            multistore_ptr_t<protocol_t> *svs,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > replier,
            backfill_session_id_t backfill_session_id,
            perfmon_collection_t *backfill_stats_parent,
            signal_t *interruptor,
            order_source_t *order_source) THROWS_ONLY(interrupted_exc_t, backfiller_lost_exc_t, broadcaster_lost_exc_t);

    /* This version of the `listener_t` constructor is called when we are
    becoming the first mirror of a new branch. It should only be called once for
    each `broadcaster_t`. */
    listener_t(
            io_backender_t *io_backender,
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
            branch_history_manager_t<protocol_t> *branch_history_manager,
            broadcaster_t<protocol_t> *broadcaster,
            perfmon_collection_t *backfill_stats_parent,
            signal_t *interruptor,
            order_source_t *order_source) THROWS_ONLY(interrupted_exc_t);

    ~listener_t();

    /* Returns a signal that is pulsed if the mirror is not in contact with the
    master. */
    signal_t *get_broadcaster_lost_signal();

    // Getters used by the replier :(
    // TODO: Some of these can and should be passed directly to the replier?
    mailbox_manager_t *mailbox_manager() const { return mailbox_manager_; }
    branch_history_manager_t<protocol_t> *branch_history_manager() const { return branch_history_manager_; }
    multistore_ptr_t<protocol_t> *svs() const {
        rassert(svs_ != NULL);
        return svs_;
    }

    branch_id_t branch_id() const {
        rassert(branch_id_ != nil_uuid());
        return branch_id_;
    }

    typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t writeread_address() const {
        return writeread_mailbox_.get_address();
    }

    typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t read_address() const {
        return read_mailbox_.get_address();
    }

    void wait_for_version(state_timestamp_t timestamp, signal_t *interruptor);

    const listener_intro_t<protocol_t> &registration_done_cond_value() const {
        return registration_done_cond_.get_value();
    }


private:
    class write_queue_entry_t {
    public:
        write_queue_entry_t() { }
        write_queue_entry_t(const typename protocol_t::write_t &w, transition_timestamp_t tt, order_token_t _order_token, fifo_enforcer_write_token_t ft) :
            write(w), transition_timestamp(tt), order_token(_order_token), fifo_token(ft) { }
        typename protocol_t::write_t write;
        transition_timestamp_t transition_timestamp;
        order_token_t order_token;
        fifo_enforcer_write_token_t fifo_token;

        // This is serializable because this gets written to a disk backed queue.
        RDB_MAKE_ME_SERIALIZABLE_4(write, order_token, transition_timestamp, fifo_token);
    };

    // TODO: This boost optional boost optional crap is ... crap.  This isn't Haskell, this is *real* programming, people.
    static boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > get_backfiller_from_replier_bcard(const boost::optional<boost::optional<replier_business_card_t<protocol_t> > > &replier_bcard);

    // TODO: Boost boost optional optional business card business card protocol_tee tee tee tee piii kaaa chuuuuu!
    static boost::optional<boost::optional<registrar_business_card_t<listener_business_card_t<protocol_t> > > > get_registrar_from_broadcaster_bcard(const boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > &broadcaster_bcard);

    /* `try_start_receiving_writes()` is called from within the constructors. It
    tries to register with the master. It throws `interrupted_exc_t` if
    `interruptor` is pulsed. Otherwise, it fills `registration_result_cond` with
    a value indicating if the registration succeeded or not, and with the intro
    we got from the broadcaster if it succeeded. */
    void try_start_receiving_writes(
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, broadcaster_lost_exc_t);

    void on_write(const typename protocol_t::write_t &write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void()> ack_addr)
        THROWS_NOTHING;

    void enqueue_write(const typename protocol_t::write_t &write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void()> ack_addr,
            auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING;

    void perform_enqueued_write(const write_queue_entry_t &serialized_write, state_timestamp_t backfill_end_timestamp, signal_t *interruptor) 
        THROWS_ONLY(interrupted_exc_t);

    /* See the note at the place where `writeread_mailbox` is declared for an
    explanation of why `on_writeread()` and `on_read()` are here. */

    void on_writeread(const typename protocol_t::write_t &write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr)
        THROWS_NOTHING;

    void perform_writeread(const typename protocol_t::write_t &write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr,
            auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING;

    void on_read(const typename protocol_t::read_t &read,
            state_timestamp_t expected_timestamp,
            order_token_t order_token,
            fifo_enforcer_read_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr)
        THROWS_NOTHING;

    void perform_read(const typename protocol_t::read_t &read,
            state_timestamp_t expected_timestamp,
            order_token_t order_token,
            fifo_enforcer_read_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr,
            auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING;

    void advance_current_timestamp_and_pulse_waiters(transition_timestamp_t timestamp);

    mailbox_manager_t *const mailbox_manager_;

    branch_history_manager_t<protocol_t> *const branch_history_manager_;

    multistore_ptr_t<protocol_t> *const svs_;

    branch_id_t branch_id_;

    /* `upgrade_mailbox` and `broadcaster_begin_timestamp` are valid only if we
    successfully registered with the broadcaster at some point. As a sanity
    check, we put them in a `promise_t`, `registration_done_cond`, that only
    gets pulsed when we successfully register. */
    promise_t<listener_intro_t<protocol_t> > registration_done_cond_;

    // This uuid exists solely as a temporary used to be passed to
    // uuid_to_str for perfmon_collection initialization and the
    // backfill queue file name
    const uuid_t uuid_;

    perfmon_collection_t perfmon_collection_;
    perfmon_membership_t perfmon_collection_membership_;

    state_timestamp_t current_timestamp_;
    fifo_enforcer_sink_t store_entrance_sink_;

    // Used by the replier_t which needs to be able to tell
    // backfillees how up to date it is.
    std::multimap<state_timestamp_t, cond_t *> synchronize_waiters_;

    disk_backed_queue_wrapper_t<write_queue_entry_t> write_queue_;
    fifo_enforcer_sink_t write_queue_entrance_sink_;
    scoped_ptr_t<typename coro_pool_t<write_queue_entry_t>::boost_function_callback_t> write_queue_coro_pool_callback_;
    adjustable_semaphore_t write_queue_semaphore_;
    cond_t write_queue_has_drained_;

    /* Destroying `write_queue_coro_pool` will stop any invocations of
    `perform_enqueued_write()`. We mustn't access any member variables defined
    below `write_queue_coro_pool` from within `perform_enqueued_write()`,
    because their destructors might have been called. */
    scoped_ptr_t<coro_pool_t<write_queue_entry_t> > write_queue_coro_pool_;

    /* This asserts that the broadcaster doesn't send us too many concurrent
    writes at the same time. */
    semaphore_assertion_t enforce_max_outstanding_writes_from_broadcaster_;

    auto_drainer_t drainer_;

    typename listener_business_card_t<protocol_t>::write_mailbox_t write_mailbox_;

    /* `writeread_mailbox` and `read_mailbox` live on the `listener_t` even
    though they don't get used until the `replier_t` is constructed. The reason
    `writeread_mailbox` has to live here is so we don't drop writes if the
    `replier_t` is destroyed without doing a warm shutdown but the `listener_t`
    stays alive. The reason `read_mailbox` is here is for consistency, and to
    have all the query-handling code in one place. */
    typename listener_business_card_t<protocol_t>::writeread_mailbox_t writeread_mailbox_;
    typename listener_business_card_t<protocol_t>::read_mailbox_t read_mailbox_;

    scoped_ptr_t<registrant_t<listener_business_card_t<protocol_t> > > registrant_;

    DISABLE_COPYING(listener_t);
};



#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_ */
