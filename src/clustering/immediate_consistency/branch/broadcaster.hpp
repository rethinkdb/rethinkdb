#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_

#include <list>
#include <map>

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/generic/registrar.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "protocol_api.hpp"
#include "timestamps.hpp"

template <class> class listener_t;
template <class> class semilattice_readwrite_view_t;
template <class> class multistore_ptr_t;
struct mailbox_manager_t;

template<class protocol_t>
class broadcaster_t : public home_thread_mixin_t {
private:
    class incomplete_write_t;

public:
    /* If the number of calls to `spawn_write()` minus the number of writes that
    have completed is equal to `MAX_OUTSTANDING_WRITES`, it's illegal to call
    `spawn_write()` again. */
    static const int MAX_OUTSTANDING_WRITES;

    class write_callback_t {
    public:
        write_callback_t();
        virtual void on_response(peer_id_t peer, const typename protocol_t::write_response_t &response) = 0;
        virtual void on_done() = 0;

    protected:
        virtual ~write_callback_t();

    private:
        friend class broadcaster_t;
        /* This is so that if the write callback is destroyed before `on_done()`
        is called, it will get deregistered. */
        incomplete_write_t *write;
    };

    broadcaster_t(
            mailbox_manager_t *mm,
            branch_history_manager_t<protocol_t> *bhm,
            multistore_ptr_t<protocol_t> *initial_svs,
            perfmon_collection_t *parent_perfmon_collection,
            order_source_t *order_source,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, fifo_enforcer_sink_t::exit_read_t *lock, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t);

    /* Unlike `read()`, `spawn_write()` returns as soon as the write has begun
    and replies asynchronously via a callback. It may block, so it takes an
    `interruptor`, but it shouldn't block for a long time. If the
    `write_callback_t` is destroyed while the write is still in progress, its
    destructor will automatically deregister it so that no segfaults will
    happen. */
    void spawn_write(typename protocol_t::write_t w, fifo_enforcer_sink_t::exit_write_t *lock, order_token_t tok, write_callback_t *cb, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    branch_id_t get_branch_id() const;

    broadcaster_business_card_t<protocol_t> get_business_card();

    MUST_USE multistore_ptr_t<protocol_t> *release_bootstrap_svs_for_listener();

private:
    class incomplete_write_ref_t;

    class shard_t;
    class dispatchee_t;
    class shard_dispatchee_t;

    void sync_until_shutdown(auto_drainer_t::lock_t);
    void sync_on_thread(int thread, state_timestamp_t go_at_least_to, state_timestamp_t *current_out);

    mailbox_manager_t *mailbox_manager;

    branch_id_t branch_id;

    /* Until our initial listener has been constructed, this holds the
    multistore_ptr that was passed to our constructor. After that,
    it's `NULL`. */
    multistore_ptr_t<protocol_t> *bootstrap_svs;

    branch_history_manager_t<protocol_t> *branch_history_manager;

    scoped_ptr_t<one_per_thread_t<shard_t> > shards;

    auto_drainer_t drainer;

    registrar_t<listener_business_card_t<protocol_t>, broadcaster_t *, dispatchee_t> registrar;

    perfmon_collection_t broadcaster_collection;
    perfmon_membership_t broadcaster_membership;

    DISABLE_COPYING(broadcaster_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_ */
