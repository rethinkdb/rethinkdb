#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/registrar.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "protocol_api.hpp"
#include "timestamps.hpp"

template <class> class listener_t;
template <class> class semilattice_readwrite_view_t;
struct mailbox_manager_t;


/* The implementation of `broadcaster_t` is a mess, but the interface is
very clean. */

template<class protocol_t>
class broadcaster_t : public home_thread_mixin_t {
public:
    broadcaster_t(
            mailbox_manager_t *mm,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history,
            store_view_t<protocol_t> *initial_store,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, fifo_enforcer_sink_t::exit_read_t *lock, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t);

    class ack_callback_t {
    public:
        /* If the return value is `true`, then `write()` will return. */
        virtual bool on_ack(peer_id_t peer) = 0;

    protected:
        virtual ~ack_callback_t() { }
    };

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, fifo_enforcer_sink_t::exit_write_t *lock, ack_callback_t *cb, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t);

    branch_id_t get_branch_id();

    broadcaster_business_card_t<protocol_t> get_business_card();

private:
    friend class listener_t<protocol_t>;

    class incomplete_write_t;

    class incomplete_write_ref_t;

    class dispatchee_t;

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. You must hold
    `dispatchee_mutex` and pass in `proof` of the mutex acquisition. (A
    dispatchee is "readable" if a `replier_t` exists for it on the remote
    machine.) */
    void pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, mutex_assertion_t::acq_t *proof, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(cannot_perform_query_exc_t);

    void background_write(dispatchee_t *, auto_drainer_t::lock_t, incomplete_write_ref_t, fifo_enforcer_write_token_t) THROWS_NOTHING;
    void end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING;

    /* This function sanity-checks `incomplete_writes`, `current_timestamp`,
    and `newest_complete_timestamp`. It mostly exists as a form of executable
    documentation. */
    void sanity_check();

    mailbox_manager_t *mailbox_manager;

    branch_id_t branch_id;

    /* Until our initial listener has been constructed, this holds the store
    that was passed to our constructor. After that, it's `NULL`. */
    store_view_t<protocol_t> *bootstrap_store;

    /* If a write has begun, but some mirror might not have completed it yet,
    then it goes in `incomplete_writes`. The idea is that a new mirror that
    connects will use the union of a backfill and `incomplete_writes` as its
    data, and that will guarantee it gets at least one copy of every write.
    See the member function `sanity_check()` for a description of the
    relationship between `incomplete_writes`, `current_timestamp`, and
    `newest_complete_timestamp`. */

    /* `mutex` is held by new writes and reads being created, by writes
    finishing, and by dispatchees joining, leaving, or upgrading. It protects
    `incomplete_writes`, `current_timestamp`, `newest_complete_timestamp`,
    `order_sink`, `dispatchees`, and `readable_dispatchees`. */
    mutex_assertion_t mutex;

    std::list<boost::shared_ptr<incomplete_write_t> > incomplete_writes;
    state_timestamp_t current_timestamp, newest_complete_timestamp;
    order_sink_t order_sink;

    std::map<dispatchee_t *, auto_drainer_t::lock_t> dispatchees;
    intrusive_list_t<dispatchee_t> readable_dispatchees;

    registrar_t<listener_business_card_t<protocol_t>, broadcaster_t *, dispatchee_t> registrar;

    struct queue_and_pool_t {
        queue_and_pool_t()
            : background_write_workers(5000, &background_write_queue, &cb)
        { }
        unlimited_fifo_queue_t<boost::function<void()> > background_write_queue;
        calling_callback_t cb;
        coro_pool_t<boost::function<void()> > background_write_workers;
    };

    boost::ptr_map<dispatchee_t *, queue_and_pool_t> coro_pools;

    DISABLE_COPYING(broadcaster_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_ */
