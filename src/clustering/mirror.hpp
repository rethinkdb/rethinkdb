#ifndef __CLUSTERING_MIRROR_HPP__
#define __CLUSTERING_MIRROR_HPP__

#include "clustering/backfillee.hpp"
#include "clustering/backfiller.hpp"
#include "clustering/mirror_metadata.hpp"
#include "clustering/registrant.hpp"
#include "concurrency/coro_fifo.hpp"
#include "concurrency/promise.hpp"
#include "rpc/metadata/view/field.hpp"
#include "rpc/metadata/view/member.hpp"
#include "timestamps.hpp"

template<class protocol_t>
class mirror_t {

private:
    typedef typename mirror_dispatcher_metadata_t<protocol_t>::mirror_data_t mirror_data_t;

public:
    /* This version of the `mirror_t` constructor is used when we are joining
    an existing branch. */

    mirror_t(
            typename protocol_t::store_t *s,
            mailbox_cluster_t *c,
            boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > > dispatcher,
            mirror_id_t backfill_provider,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t) :

        store(s),
        cluster(c),

        write_mailbox(cluster, boost::bind(&mirror_t::on_write, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&mirror_t::on_writeread, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&mirror_t::on_read, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4))
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        /* Attempt to register for reads and writes */
        state_timestamp_t initial_timestamp;
        typename mirror_data_t::upgrade_mailbox_t::address_t upgrade_mailbox;
        try {
            start_receiving_writes(dispatcher, interruptor, &initial_timestamp, &upgrade_mailbox);
        } catch (resource_lost_exc_t) {
            rassert(!registrant || registrant->get_failed_signal()->is_pulsed());
        }

        /* Get a backfill. This can throw `resource_lost_exc_t` or
        `interrupted_exc_t`; we pass both on. */
        backfill<protocol_t>(
            store, cluster,
            metadata_member(backfill_provider, metadata_field(&mirror_dispatcher_metadata_t<protocol_t>::mirrors, dispatcher)),
            interruptor);

        if (registrant && !registrant->get_failed_signal()->is_pulsed() && initial_timestamp > store->get_timestamp()) {
            /* The node we got our backfill from was outdated, so we're
            outdated too. Disconnect from the master. */
            registrant.reset();
        }

        if (registrant && !registrant->get_failed_signal()->is_pulsed()) {
            /* Allow write operations to proceed */
            allow_writes_cond.pulse(store->get_timestamp());

            /* Register for writereads and reads */
            send(cluster, upgrade_mailbox,
                writeread_mailbox.get_address(), read_mailbox.get_address());

        } else {
            /* Cancel any write operations that tried to start */
            cancel_writes_cond.pulse();
        }

        start_serving_backfills(dispatcher);
    }

    /* This version of the `mirror_t` constructor is used when we are becoming
    the first mirror of a new branch. */

    mirror_t(
            typename protocol_t::store_t *s,
            mailbox_cluster_t *c,
            boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > > dispatcher,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) :

        store(s),
        cluster(c),

        write_mailbox(cluster, boost::bind(&mirror_t::on_write, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&mirror_t::on_writeread, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&mirror_t::on_read, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4))
    {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();

        state_timestamp_t initial_timestamp;
        typename mirror_data_t::upgrade_mailbox_t::address_t upgrade_mailbox;
        try {
            start_receiving_writes(dispatcher, interruptor, &initial_timestamp, &upgrade_mailbox);
        } catch (resource_lost_exc_t) {
            rassert(!registrant || registrant->get_failed_signal()->is_pulsed());
        }

        if (registrant && !registrant->get_failed_signal()->is_pulsed()) {
            rassert(initial_timestamp == store->get_timestamp());
            allow_writes_cond.pulse(store->get_timestamp());
            /* Register for writereads and reads */
            send(cluster, upgrade_mailbox,
                writeread_mailbox.get_address(), read_mailbox.get_address());
        } else {
            cancel_writes_cond.pulse();
        }

        start_serving_backfills(dispatcher);
    }

    ~mirror_t() {

        /* Here's what happens when the `mirror_t` is destroyed:

        `backfiller`'s destructor is run. We stop providing backfills to other
        nodes and interrupt any existing backfills. We also change the metadata
        to reflect that we're no longer backfilling.

        `registrant`'s destructor is run. We tell the master that we're no
        longer handling queries.

        `write_mailbox`, `writeread_mailbox`, and `read_mailbox` are the next to
        go. Any queries from the master that were en route when we destroyed
        `registrant` will be shut out if they haven't arrived yet.

        Finally, `drainer`'s destructor is run. It pulses `get_drain_signal()`,
        which interrupts any running queries. Then it blocks until all running
        queries have stopped. */
    }

    mirror_id_t get_mirror_id() {
        return mirror_id;
    }

    /* Returns a signal that is pulsed if the mirror is not in contact with the
    master. */
    signal_t *get_outdated_signal() {
        if (registrant) {
            return registrant->get_failed_signal();
        } else {
            return &always_pulsed_signal;
        }
    }

private:
    /* Support class for `start_receiving_writes()`. It's defined out here, as
    opposed to locally in `start_receiving_writes()`, so that we can
    `boost::bind()` to it. */
    class intro_receiver_t : public signal_t {
    public:
        state_timestamp_t initial_timestamp;
        typename mirror_data_t::upgrade_mailbox_t::address_t upgrade_mailbox;
        void fill(state_timestamp_t its, typename mirror_data_t::upgrade_mailbox_t::address_t um) {
            rassert(!is_pulsed());
            initial_timestamp = its;
            upgrade_mailbox = um;
            pulse();
        }
    };

    /* `start_receiving_writes()` is called from within the constructors. It
    throws `resource_lost_exc_t` if we can't register with the master, or
    `interrupted_exc_t` if `interruptor` is pulsed. If all goes well, it fills
    `*upgrade_mailbox_out` with the address of the upgrade mailbox that the
    master gave us. */
    void start_receiving_writes(
            boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > > dispatcher,
            signal_t *interruptor,
            state_timestamp_t *initial_timestamp_out, typename mirror_data_t::upgrade_mailbox_t::address_t *upgrade_mailbox_out)
            THROWS_ONLY(resource_lost_exc_t, interrupted_exc_t)
    {
        intro_receiver_t intro_receiver;
        typename mirror_data_t::intro_mailbox_t intro_mailbox(cluster, boost::bind(&intro_receiver_t::fill, &intro_receiver, _1, _2));

        registrant.reset(new registrant_t<mirror_data_t>(
            cluster,
            metadata_field(&mirror_dispatcher_metadata_t<protocol_t>::registrar, dispatcher),
            mirror_data_t(intro_mailbox.get_address(), write_mailbox.get_address())
            ));

        wait_any_t waiter(&intro_receiver, interruptor, registrant->get_failed_signal());
        waiter.wait_lazily_unordered();
        if (interruptor->is_pulsed()) throw interrupted_exc_t();
        if (registrant->get_failed_signal()->is_pulsed()) throw resource_lost_exc_t();
        rassert(intro_receiver.is_pulsed());

        *initial_timestamp_out = intro_receiver.initial_timestamp;
        *upgrade_mailbox_out = intro_receiver.upgrade_mailbox;
    }

    /* `start_serving_backfills()` is called from within the constructors. It
    only exists to factor out some code shared between the constructors. */
    void start_serving_backfills(
            boost::shared_ptr<metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > > dispatcher)
            THROWS_NOTHING
    {
        rassert(backfiller.get() == NULL);

        /* Pick a mirror ID */
        mirror_id = generate_uuid();

        /* Set up the actual backfiller */
        backfiller.reset(new backfiller_t<protocol_t>(
            cluster,
            store,
            metadata_new_member(mirror_id, metadata_field(&mirror_dispatcher_metadata_t<protocol_t>::mirrors, dispatcher))
            ));
    }

    void on_write(auto_drainer_t::lock_t keepalive,
            typename protocol_t::write_t write, transition_timestamp_t ts, order_token_t tok,
            async_mailbox_t<void()>::address_t ack_addr)
            THROWS_NOTHING
    {
        try {
            {
                /* Wait until writes are allowed */
                coro_fifo_acq_t order_preserver;
                order_preserver.enter(&operation_order_manager);
                wait_any_t waiter(allow_writes_cond.get_ready_signal(), &cancel_writes_cond, keepalive.get_drain_signal());
                waiter.wait_lazily_unordered();
                order_preserver.leave();
            }
            if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();
            if (cancel_writes_cond.is_pulsed()) {
                /* Something went wrong; we changed our mind about accepting
                writes. */
            }

            /* Compare with the backfill cutoff so that we don't double-perform
            operations that we get from both the backfiller and the master */
            if (ts.timestamp_before() >= allow_writes_cond.get_value()) {
                rassert(ts.timestamp_before() == store->get_timestamp(), "write "
                    "got reordered w.r.t. write.");

                store->write(write, ts, tok, keepalive.get_drain_signal());
            }

            send(cluster, ack_addr);

        } catch (interrupted_exc_t) {
            /* Ignore; it means we're shutting down, so we don't need to respond
            to the query. */
        }
    }

    void on_writeread(auto_drainer_t::lock_t keepalive,
            typename protocol_t::write_t write, transition_timestamp_t ts, order_token_t tok,
            typename async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t resp_addr)
            THROWS_NOTHING
    {
        try {
            rassert(allow_writes_cond.get_ready_signal()->is_pulsed());
            if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

            rassert(ts.timestamp_before() == store->get_timestamp(), "write "
                "got reordered w.r.t. write.");

            typename protocol_t::write_response_t resp = store->write(write, ts, tok, keepalive.get_drain_signal());
            send(cluster, resp_addr, resp);

        } catch (interrupted_exc_t) {
            /* Ignore */
        }
    }

    void on_read(auto_drainer_t::lock_t keepalive,
            typename protocol_t::read_t read, state_timestamp_t ts, order_token_t tok,
            typename async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t resp_addr)
            THROWS_NOTHING
    {
        try {
            rassert(allow_writes_cond.get_ready_signal()->is_pulsed());
            if (keepalive.get_drain_signal()->is_pulsed()) throw interrupted_exc_t();

            rassert(ts == store->get_timestamp(), "read got reordered w.r.t. "
                "write.");

            typename protocol_t::read_response_t resp = store->read(read, tok, keepalive.get_drain_signal());
            send(cluster, resp_addr, resp);

        } catch (interrupted_exc_t) {
            /* Ignore */
        }
    }

    typename protocol_t::store_t *store;
    mailbox_cluster_t *cluster;

    /* This `coro_fifo_t` is responsible for making sure that operations are
    run in the same order that they arrive. We also use it to make the
    writes wait until the backfill is done. */
    coro_fifo_t operation_order_manager;

    promise_t<state_timestamp_t> allow_writes_cond;
    cond_t cancel_writes_cond;

    auto_drainer_t drainer;

    typename mirror_data_t::write_mailbox_t write_mailbox;
    typename mirror_data_t::writeread_mailbox_t writeread_mailbox;
    typename mirror_data_t::read_mailbox_t read_mailbox;

    boost::scoped_ptr<registrant_t<mirror_data_t> > registrant;

    /* If we never successfully registered, then `get_outdated_signal()` returns
    `&always_pulsed_signal`. */
    struct always_pulsed_signal_t : public signal_t {
        always_pulsed_signal_t() { pulse(); }
    } always_pulsed_signal;

    mirror_id_t mirror_id;
    boost::scoped_ptr<backfiller_t<protocol_t> > backfiller;
};

#endif /* __CLUSTERING_MIRROR_HPP__ */
