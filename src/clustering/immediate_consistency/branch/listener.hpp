#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_

#include <map>

#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/semantic_checking.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/registrant.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "serializer/types.hpp"
#include "timestamps.hpp"
#include "utils.hpp"

template <class T> class replier_t;
template <class T> class intro_receiver_t;
template <class T> class registrant_t;
template <class T> class semilattice_read_view_t;
template <class T> class semilattice_readwrite_view_t;
template <class T> class broadcaster_t;

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

template<class protocol_t>
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
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
            boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > bh,
            multistore_ptr_t<protocol_t> *svs,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<replier_business_card_t<protocol_t> > > > > replier,
            backfill_session_id_t backfill_session_id,
            perfmon_collection_t *backfill_stats_parent,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, backfiller_lost_exc_t, broadcaster_lost_exc_t);

    /* This version of the `listener_t` constructor is called when we are
    becoming the first mirror of a new branch. It should only be called once for
    each `broadcaster_t`. */
    listener_t(
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster_metadata,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > bh,
            broadcaster_t<protocol_t> *broadcaster,
            perfmon_collection_t *backfill_stats_parent,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    /* Returns a signal that is pulsed if the mirror is not in contact with the
    master. */
    signal_t *get_broadcaster_lost_signal();

private:
    friend class replier_t<protocol_t>;
    friend class intro_receiver_t<protocol_t>;

    /* `intro_t` represents the introduction we expect to get from the
    broadcaster if all goes well. */
    class intro_t {
    public:
        typename listener_business_card_t<protocol_t>::upgrade_mailbox_t::address_t upgrade_mailbox;
        typename listener_business_card_t<protocol_t>::downgrade_mailbox_t::address_t downgrade_mailbox;
        state_timestamp_t broadcaster_begin_timestamp;
    };

    class write_queue_entry_t {
    public:
        // TODO: Can we not remove the default constructor?
        write_queue_entry_t() { }
        write_queue_entry_t(const typename protocol_t::write_t &w, transition_timestamp_t tt, order_token_t _order_token, fifo_enforcer_write_token_t ft) :
            write(w), transition_timestamp(tt), order_token(_order_token), fifo_token(ft) { }
        typename protocol_t::write_t write;
        transition_timestamp_t transition_timestamp;
        order_token_t order_token;
        fifo_enforcer_write_token_t fifo_token;

        // TODO: Why does this need to be serializable?
        RDB_MAKE_ME_SERIALIZABLE_4(write, order_token, transition_timestamp, fifo_token);
    };

    // TODO: What the fuck is this boost optional boost optional shit?
    static boost::optional<boost::optional<backfiller_business_card_t<protocol_t> > > get_backfiller_from_replier_bcard(const boost::optional<boost::optional<replier_business_card_t<protocol_t> > > &replier_bcard);

    // TODO: Holy motherfucking fuck what in the name of Fuck is this
    // boost optional boost optional shit?
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

    void on_write(typename protocol_t::write_t write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void()> ack_addr)
        THROWS_NOTHING;

    void enqueue_write(typename protocol_t::write_t write,
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

    void on_writeread(typename protocol_t::write_t write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr)
        THROWS_NOTHING;

    void perform_writeread(typename protocol_t::write_t write,
            transition_timestamp_t transition_timestamp,
            order_token_t order_token,
            fifo_enforcer_write_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::write_response_t)> ack_addr,
            auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING;

    void on_read(typename protocol_t::read_t read,
            state_timestamp_t expected_timestamp,
            order_token_t order_token,
            fifo_enforcer_read_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr)
        THROWS_NOTHING;

    void perform_read(typename protocol_t::read_t read,
            DEBUG_ONLY_VAR state_timestamp_t expected_timestamp,
            order_token_t order_token,
            fifo_enforcer_read_token_t fifo_token,
            mailbox_addr_t<void(typename protocol_t::read_response_t)> ack_addr,
            auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING;

    void wait_for_version(state_timestamp_t timestamp, signal_t *interruptor);

    void advance_current_timestamp_and_pulse_waiters(transition_timestamp_t timestamp);

    mailbox_manager_t *mailbox_manager;

    boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history;

    multistore_ptr_t<protocol_t> *svs;

    branch_id_t branch_id;

    /* `upgrade_mailbox` and `broadcaster_begin_timestamp` are valid only if we
    successfully registered with the broadcaster at some point. As a sanity
    check, we put them in a `promise_t`, `registration_done_cond`, that only
    gets pulsed when we successfully register. */
    promise_t<intro_t> registration_done_cond;

    boost::uuids::uuid uuid;
    perfmon_collection_t perfmon_collection;
    fifo_enforcer_sink_t write_queue_entrance_sink;
    disk_backed_queue_wrapper_t<write_queue_entry_t> write_queue;
    boost::scoped_ptr<typename coro_pool_t<write_queue_entry_t>::boost_function_callback_t> write_queue_coro_pool_callback;
    boost::scoped_ptr<coro_pool_t<write_queue_entry_t> > write_queue_coro_pool;
    adjustable_semaphore_t write_queue_semaphore;
    cond_t write_queue_has_drained;

    state_timestamp_t current_timestamp;

    fifo_enforcer_sink_t store_entrance_sink;

    auto_drainer_t drainer;

    typename listener_business_card_t<protocol_t>::write_mailbox_t write_mailbox;

    /* `writeread_mailbox` and `read_mailbox` live on the `listener_t` even
    though they don't get used until the `replier_t` is constructed. The reason
    `writeread_mailbox` has to live here is so we don't drop writes if the
    `replier_t` is destroyed without doing a warm shutdown but the `listener_t`
    stays alive. The reason `read_mailbox` is here is for consistency, and to
    have all the query-handling code in one place. */
    typename listener_business_card_t<protocol_t>::writeread_mailbox_t writeread_mailbox;
    typename listener_business_card_t<protocol_t>::read_mailbox_t read_mailbox;

    boost::scoped_ptr<registrant_t<listener_business_card_t<protocol_t> > > registrant;

    /* Avaste this be used to keep track of people who are waitin' for a us to
     * be up to date (past the given state_timestamp_t). The only use case for
     * this right now is the replier_t who needs to be able to tell backfillees
     * how up to date s/he is. */
    std::multimap<state_timestamp_t, cond_t *> synchronize_waiters;

    DISABLE_COPYING(listener_t);
};



#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_LISTENER_HPP_ */
