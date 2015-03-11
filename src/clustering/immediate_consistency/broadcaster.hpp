// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BROADCASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BROADCASTER_HPP_

#include <list>
#include <map>
#include <set>
#include <vector>

#include "utils.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/generic/registrar.hpp"
#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/metadata.hpp"
#include "concurrency/min_timestamp_enforcer.hpp"
#include "concurrency/queue/unlimited_fifo.hpp"
#include "concurrency/watchable.hpp"
#include "timestamps.hpp"

class listener_t;
template <class> class semilattice_readwrite_view_t;
class mailbox_manager_t;
class rdb_context_t;
class uuid_u;

/* Each shard has a `broadcaster_t` on its primary replica. Each server sends
queries via `cluster_namespace_interface_t` over the network to the `master_t`
on the primary replica server, which forwards the queries to the `broadcaster_t`.
From there, the `broadcaster_t` distributes them to one or more `listener_t`s.

When the `broadcaster_t` is first created, it generates a new unique branch ID.
The `broadcaster_t` is the authority on what sequence of operations is performed
on that branch. The order in which write and read operations pass through the
`broadcaster_t` is the order in which they are performed at the B-trees
themselves. */

class broadcaster_t : public home_thread_mixin_debug_only_t {
private:
    class incomplete_write_t;

public:
    class write_callback_t {
    public:
        write_callback_t();
        /* `get_default_write_durability()` returns the write durability that this write
        should use if the write itself didn't specify. */
        virtual write_durability_t get_default_write_durability() = 0;
        /* Every time the write is acked, `on_ack()` is called. When no more replicas
        will ack the write, `on_end()` is called. */
        virtual void on_ack(const server_id_t &, write_response_t &&) = 0;
        virtual void on_end() = 0;

    protected:
        virtual ~write_callback_t();

    private:
        friend class broadcaster_t;
        /* This is so that if the write callback is destroyed before `on_end()` is
        called, it will get deregistered. */
        incomplete_write_t *write;
    };

    broadcaster_t(
            mailbox_manager_t *mm,
            branch_history_manager_t *bhm,
            store_view_t *initial_svs,
            perfmon_collection_t *parent_perfmon_collection,
            const branch_id_t &branch_id,
            const branch_birth_certificate_t &branch_info,
            order_source_t *order_source,   /* only used during the constructor */
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    void read(
        const read_t &r,
        read_response_t *response,
        fifo_enforcer_sink_t::exit_read_t *lock,
        order_token_t tok,
        signal_t *interruptor)
        THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t);

    /* Unlike `read()`, `spawn_write()` returns as soon as the write has begun and
    replies asynchronously via a callback. It will not block. If the `write_callback_t`
    is destroyed while the write is still in progress, its destructor will automatically
    deregister it so that no segfaults will happen. */
    void spawn_write(const write_t &w,
                     order_token_t tok,
                     write_callback_t *cb);

    branch_id_t get_branch_id() const;
    region_t get_region();
    broadcaster_business_card_t get_business_card();

    clone_ptr_t<watchable_t<std::set<server_id_t> > > get_readable_dispatchees() {
        return readable_dispatchees_as_set.get_watchable();
    }

    MUST_USE store_view_t *release_bootstrap_svs_for_listener();

    void register_local_listener(const uuid_u &listener_id,
                                 listener_t *listener,
                                 auto_drainer_t::lock_t listener_keepalive);

private:
    class incomplete_write_ref_t;

    class dispatchee_t;

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. You must hold
    `dispatchee_mutex` and pass in `proof` of the mutex acquisition. (A
    dispatchee is "readable" if a `replier_t` exists for it on the remote
    server.) */
    void pick_a_readable_dispatchee(
        dispatchee_t **dispatchee_out, mutex_assertion_t::acq_t *proof,
        auto_drainer_t::lock_t *lock_out) THROWS_ONLY(cannot_perform_query_exc_t);

    void background_write(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token) THROWS_NOTHING;
    void background_writeread(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token, write_durability_t durability) THROWS_NOTHING;
    void end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING;

    void listener_read(
        broadcaster_t::dispatchee_t *mirror,
        const read_t &r,
        read_response_t *response,
        min_timestamp_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void listener_write(
        broadcaster_t::dispatchee_t *mirror,
        const write_t &w, state_timestamp_t timestamp,
        order_token_t order_token, fifo_enforcer_write_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    /* This recomputes `readable_dispatchees_as_set()` from `readable_dispatchees()` */
    void refresh_readable_dispatchees_as_set();

    /* This function sanity-checks `incomplete_writes`, `current_timestamp`,
    and `newest_complete_timestamp`. It mostly exists as a form of executable
    documentation. */
    void sanity_check();

    perfmon_collection_t broadcaster_collection;
    perfmon_membership_t broadcaster_membership;

    mailbox_manager_t *const mailbox_manager;
    const region_t region;
    const branch_id_t branch_id;

    /* Until our initial listener has been constructed, this holds the
    store_view that was passed to our constructor. After that, it's
    `NULL`. */
    store_view_t *bootstrap_svs;

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
    `order_checkpoint`, `dispatchees`, and `readable_dispatchees`. */
    mutex_assertion_t mutex;

    std::list<boost::shared_ptr<incomplete_write_t> > incomplete_writes;
    state_timestamp_t current_timestamp, newest_complete_timestamp;
    order_checkpoint_t order_checkpoint;

    /* Once we ack a write, we must make sure that every read that's initiated
    after that will see the result of the write. We use this timestamp to keep
    track of the most recent acked write and produce `min_timestamp_token_t`s from
    it whenever we send a read to a listener. */
    state_timestamp_t most_recent_acked_write_timestamp;

    std::map<dispatchee_t *, auto_drainer_t::lock_t> dispatchees;
    intrusive_list_t<dispatchee_t> readable_dispatchees;

    /* This is just a set that contains the peer ID of each dispatchee in
    `readable_dispatchees`. We store it separately so we can expose it to code that needs
    to know which replicas are available. */
    watchable_variable_t<std::set<server_id_t> > readable_dispatchees_as_set;

    registrar_t<listener_business_card_t, broadcaster_t *, dispatchee_t>
        registrar;

    DISABLE_COPYING(broadcaster_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BROADCASTER_HPP_ */
