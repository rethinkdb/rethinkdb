#ifndef __CLUSTERING_LISTENER_HPP__
#define __CLUSTERING_LISTENER_HPP__

#include "clustering/branch_metadata.hpp"
#include "clustering/resource.hpp"
#include "clustering/registrant.hpp"
#include "protocol_api.hpp"

/* `listener_t` keeps a store-view in sync with a branch. Its constructor
backfills from an existing mirror on a branch into the store, and as long as it
exists the store will receive real-time updates. */

template<class protocol_t>
class listener_t {

public:
    listener_t(
            mailbox_cluster_t *c,
            store_view_t<protocol_t> *s,
            boost::shared_ptr<metadata_read_view_t<std::map<branch_id_t, branch_metadata_t<protocol_t> > > > branches_metadata,
            branch_id_t bid,
            mirror_id_t backfiller_id,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t, resource_lost_exc_t) :

        cluster(c),
        store(s),

        write_mailbox(cluster, boost::bind(&listener_t::on_write, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&listener_t::on_writeread, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&listener_t::on_read, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),

        branch_id(bid)
    {
        /* Validate input */
        rassert(branches_metadata->get().count(branch_id) > 0);
        rassert(region_is_superset(branches_metadata->get()[branch_id].region, store->get_region()));

        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }

        /* Attempt to register for reads and writes */
        boost::shared_ptr<metadata_read_view_t<branch_metadata_t<protocol_t> > > branch_metadata =
            metadata_member(branch_id, branches_metadata);
        try_start_receiving_writes(branch_metadata, interruptor);

        /* Backfill */
        

        /* If there was a data gap, update the outdated-signal accordingly. This
        must be done before we return from the constructor. */
        if (registration_result_cond.get_value() &&
                backfill_done_cond.get_value().backfill_end_timestamp <
                registration_result_cond.get_value()->broadcaster_begin_timestamp) {
            data_gap_cond.pulse();
            outdated_signal.add(&data_gap_cond);
        }
    }

    /* Returns a signal that is pulsed if the mirror is not in contact with the
    master. */
    signal_t *get_outdated_signal() {
        return &outdated_signal;
    }

private:
    friend class backfiller_t;
    friend class master_t;
    friend class replier_t;

    /* Support class for `start_receiving_writes()`. It's defined out here, as
    opposed to locally in `start_receiving_writes()`, so that we can
    `boost::bind()` to it. */
    class intro_receiver_t : public signal_t {
    public:
        registration_succeeded_t intro;
        void fill(state_timestamp_t its,
                typename listener_data_t<protocol_t>::upgrade_mailbox_t::address_t um,
                listener_data_t<protocol_t>::downgrade_mailbox_t::address_t dm) {
            rassert(!is_pulsed());
            intro.broadcaster_begin_timestamp = its;
            intro.upgrade_mailbox = um;
            intro.downgrade_mailbox = dm;
            pulse();
        }
    };

    /* This version of the `listener_t` constructor is called when we are
    becoming the first mirror of a new branch. It's private because it shouldn't
    be used for any other purpose. */
    listener_t(
            mailbox_cluster_t *c,
            store_view_t<protocol_t> *s,
            boost::shared_ptr<metadata_read_view_t<branch_metadata_t<protocol_t> > > branch_metadata,
            branch_id_t bid,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) :

        cluster(c),
        store(s),

        write_mailbox(cluster, boost::bind(&listener_t::on_write, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        writeread_mailbox(cluster, boost::bind(&listener_t::on_writeread, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),
        read_mailbox(cluster, boost::bind(&listener_t::on_read, this,
            auto_drainer_t::lock_t(&drainer), _1, _2, _3, _4)),

        branch_id(bid)
    {
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }

        /* Make sure the initial metadata is sane */
        region_map_t<typename protocol_t::region_t, binary_blob_t> initial_metadata =
            store->begin_read_transaction(interruptor)->get_metadata(interruptor);
        rassert(initial_metadata.get_as_pairs().size() == 1);
        version_range_t initial_version = initial_metadata.get_as_pairs()[0].second;
        rassert(initial_version.is_coherent());
        rassert(initial_version.earliest.branch == branch_id);
        state_timestamp_t initial_timestamp = initial_version.earliest.timestamp;

        /* Attempt to register for reads and writes */
        try_start_receiving_writes(branch_metadata, interruptor);

        /* Make sure that `broadcast_begin_timestamp` matches the initial
        metadata */
        if (registration_result_cond.get_value()) {
            rassert(registration_result_cond.get_value().broadcast_begin_timestamp ==
                    initial_timestamp);
        }

        /* Pretend we just finished an imaginary backfill */
        backfill_succeeded_t fake_success_record;
        fake_success_record.backfill_end_timestamp = initial_timestamp;
        backfill_done_cond.pulse(fake_success_record);

        /* When operating in this mode, the only reason we can become outdated
        is if we lose contact with the broadcaster. There cannot possibly be a
        data gap, and `data_gap_cond` is unused. */
    }

    /* `try_start_receiving_writes()` is called from within the constructors. It
    tries to register with the master. It throws `interrupted_exc_t` if
    `interruptor` is pulsed. Otherwise, it fills `registration_result_cond` with
    a value indicating if the registration succeeded or not, and with the intro
    we got from the broadcaster if it succeeded. */
    void try_start_receiving_writes(
            boost::shared_ptr<metadata_readwrite_view_t<branch_metadata_t<protocol_t> > > dispatcher,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t)
    {
        intro_receiver_t intro_receiver;
        typename listener_data_t<protocol_t>::intro_mailbox_t intro_mailbox(
            cluster, boost::bind(&intro_receiver_t::fill, &intro_receiver, _1, _2, _3));

        try {
            registrant.reset(new registrant_t<listener_data_t<protocol_t> >(
                cluster,
                metadata_field(&mirror_dispatcher_metadata_t<protocol_t>::registrar, dispatcher),
                listener_data_t<protocol_t>(intro_mailbox.get_address(), write_mailbox.get_address())
                ));
        } catch (resource_lost_exc_t) {
            registration_result_cond.pulse(boost::optional<registration_succeeded_t>());
        }

        wait_any_t waiter(&intro_receiver, interruptor, registrant->get_failed_signal());
        waiter.wait_lazily_unordered();
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }

        if (registrant->get_failed_signal()->is_pulsed()) {
            registration_result_cond.pulse(boost::optional<registration_succeeded_t>());
        } else {
            rassert(intro_receiver.is_pulsed());
            registration_result_cond.pulse(boost::optional<registration_succeeded_t>(
                intro_receiver.intro));
        }

        /* If we lose contact with the master, mark ourselves outdated */
        outdated_signal.add(registrant->get_failed_signal()->is_pulsed());
    }

    void on_write(auto_drainer_t::lock_t keepalive,
            typename protocol_t::write_t write, transition_timestamp_t ts, order_token_t tok,
            async_mailbox_t<void()>::address_t ack_addr)
            THROWS_NOTHING
    {
        coro_fifo_acq_t order_preserver;
        order_preserver.enter(&operation_order_manager);

        /* Block until registration has completely succeeded or failed. */
        {
            wait_any_t waiter(registration_result_cond.get_ready_signal(), keepalive.get_drain_signal());
            waiter.wait_lazily_unordered();
            if (keepalive.get_drain_signal()->is_pulsed()) {
                return;
            }
        }

        if (!registration_result_cond.get_value()) {
            /* Registration never succeeded; for some reason a few writes were
            transmitted anyway, and we're one of them. Abort. */
            return;
        }

        /* Block until the backfill succeeds or fails. */
        {
            wait_any_t waiter(backfill_done_cond.get_ready_signal(), keepalive.get_drain_signal());
            waiter.wait_lazily_unordered();
            if (keepalive.get_drain_signal()->is_pulsed()) {
                return;
            }
        }

        if (backfill_done_cond.get_value().backfill_end_timestamp <
                registration_result_cond.get_value()->broadcaster_begin_timestamp) {
            /* There was a gap between the end of the backfill and the beginning
            of the real-time streaming operations. The only way to stay
            consistent is to throw away all the real-time streaming operations
            and mark ourselves outdated. */
            return;
        }

        if (ts.timestamp_before() < backfill_done_cond.get_value()->backfill_end_timestamp) {
            /* `write` is a duplicate; we got it both as part of the backfill,
            and from the broadcaster. Ignore this copy of it. */
            return;
        }

        /* TODO: This is fragile. The key-value store should use the timestamps
        for ordering and should be durable against `write()` being called out of
        order. */
        order_preserver.leave();

        /* Perform the operation */
        try {
            backfill_done_cond.get_value()->ready_store->write(
                write, ts, tok, keepalive.get_drain_signal());
        } catch (interrupted_exc_t) {
            /* We're shutting down; don't bother finishing this operation. */
            return;
        }

        send(cluster, ack_addr);
    }

    /* See the note at the place where `writeread_mailbox` is declared for an
    explanation of why `on_writeread()` and `on_read()` are here. */

    void on_writeread(auto_drainer_t::lock_t keepalive,
            typename protocol_t::write_t write, transition_timestamp_t ts, order_token_t tok,
            typename async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t ack_addr)
            THROWS_NOTHING
    {
        /* Even though we aren't doing anything that could block, go through the
        order preserver anyway to maintain ordering relative to calls to
        `on_write()` */
        coro_fifo_acq_t order_preserver;
        order_preserver.enter(&operation_order_manager);

        /* We can't possibly be receiving writereads unless we successfully
        registered and completed a backfill, and we are not outdated */
        rassert(registration_result_cond.get_ready_signal()->is_pulsed() &&
            registration_result_cond.get_value());
        rassert(backfill_done_cond.get_ready_signal()->is_pulsed());
        rassert(!data_gap_cond.is_pulsed());

        /* This mustn't be a duplicate operation because we can't register for
        writereads until the backfill is over */
        rassert(ts.timestamp_before() >= backfill_done_cond.get_value()->backfill_end_timestamp);

        order_preserver.leave();

        /* Perform the operation */
        typename protocol_t::write_response_t write_response;
        try {
            write_response = backfill_done_cond.get_value()->ready_store->write(
                write, ts, tok, keepalive.get_drain_signal());
        } catch (interrupted_exc_t) {
            /* We're shutting down; don't bother finishing this operation. */
            return;
        }

        send(cluster, ack_addr, write_response);
    }

    void on_read(auto_drainer_t::lock_t keepalive,
            typename protocol_t::read_t read, state_timestamp_t ts, order_token_t tok,
            typename async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t ack_addr)
            THROWS_NOTHING
    {
        /* Even though we aren't doing anything that could block, go through the
        order preserver anyway to maintain ordering relative to calls to
        `on_write()` */
        coro_fifo_acq_t order_preserver;
        order_preserver.enter(&operation_order_manager);

        /* We can't possibly be receiving reads unless we successfully
        registered and completed a backfill, and we are not outdated */
        rassert(registration_result_cond.get_ready_signal()->is_pulsed() &&
            registration_result_cond.get_value());
        rassert(backfill_done_cond.get_ready_signal()->is_pulsed());
        rassert(!data_gap_cond.is_pulsed());

        /* This mustn't be a duplicate operation because we can't register for
        reads until the backfill is over */
        rassert(ts.timestamp_before() >= backfill_done_cond.get_value()->backfill_end_timestamp);

        order_preserver.leave();

        /* Perform the operation */
        typename protocol_t::read_response_t read_response;
        try {
            read_response = backfill_done_cond.get_value()->ready_store->read(
                read, ts, tok, keepalive.get_drain_signal());
        } catch (interrupted_exc_t) {
            /* We're shutting down; don't bother finishing this operation. */
            return;
        }

        send(cluster, ack_addr, read_response);
    }

    mailbox_cluster_t *cluster;

    store_view_t<protocol_t> *store;

    /* `upgrade_mailbox` and `broadcaster_begin_timestamp` are valid only if we
    successfully registered with the broadcaster at some point. As a sanity
    check, we put them in a `promise_t` that only gets pulsed when we
    successfully register. If registration fails, we pulse
    `registration_result_cond` with an empty `boost::optional`. */
    class registration_succeeded_t {
    public:
        typename listener_data_t<protocol_t>::upgrade_mailbox_t::address_t upgrade_mailbox;
        listener_data_t<protocol_t>::downgrade_mailbox_t::address_t downgrade_mailbox;
        state_timestamp_t broadcaster_begin_timestamp;
    };
    promise_t<boost::optional<registration_succeeded_t> > registration_result_cond;

    /* This `coro_fifo_t` is responsible for making sure that operations are
    run in the same order that they arrive. We also use it to make the
    writes wait until the backfill is done. */
    coro_fifo_t operation_order_manager;

    /* `ready_store` and `backfill_end_timestamp` aren't valid until the
    backfill is over. As a sanity check, we put them in a `promise_t` that only
    gets pulsed after the backfill is over. If the backfill fails, we never
    pulse `backfill_done_cond`. */
    class backfill_succeeded_t {
    public:
        boost::shared_ptr<ready_store_t<protocol_t> > ready_store;
        state_timestamp_t backfill_end_timestamp;
    };
    promise_t<backfill_succeeded_t> backfill_done_cond;

    auto_drainer_t drainer;

    typename listener_data_t<protocol_t>::write_mailbox_t write_mailbox;

    /* `writeread_mailbox` and `read_mailbox` live on the `listener_t` even
    though they don't get used until the `replier_t` is constructed. The reason
    `writeread_mailbox` has to live here is so we don't drop writes if the
    `replier_t` is destroyed without doing a warm shutdown but the `listener_t`
    stays alive. The reason `read_mailbox` is here is for consistency, and to
    have all the query-handling code in one place. */
    typename listener_data_t<protocol_t>::writeread_mailbox_t writeread_mailbox;
    typename listener_data_t<protocol_t>::read_mailbox_t read_mailbox;

    branch_id_t branch_id;
    boost::scoped_ptr<registrant_t<listener_data_t<protocol_t> > > registrant;

    /* `data_gap_cond` gets pulsed if the backfiller is outdated.
    `outdated_signal` is pulsed if we become outdated for any reason. */
    cond_t data_gap_cond;
    wait_any_t outdated_signal;
};