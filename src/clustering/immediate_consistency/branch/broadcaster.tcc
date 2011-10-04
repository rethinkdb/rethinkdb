#include "concurrency/coro_fifo.hpp"

/* Functions to send a read or write to a mirror and wait for a response. */

template<class protocol_t>
void listener_write(
        mailbox_cluster_t *cluster,
        const typename listener_data_t<protocol_t>::write_mailbox_t::address_t &write_mailbox,
        typename protocol_t::write_t w, transition_timestamp_t ts,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    cond_t ack_cond;
    async_mailbox_t<void()> ack_mailbox(
        cluster, boost::bind(&cond_t::pulse, &ack_cond));

    send(cluster, write_mailbox,
        w, ts, ack_mailbox.get_address());

    wait_any_t waiter(&ack_cond, interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    rassert(ack_cond.is_pulsed());
}

template<class protocol_t>
typename protocol_t::write_response_t listener_writeread(
        mailbox_cluster_t *cluster,
        const typename listener_data_t<protocol_t>::writeread_mailbox_t::address_t &writeread_mailbox,
        typename protocol_t::write_t w, transition_timestamp_t ts,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    promise_t<typename protocol_t::write_response_t> resp_cond;
    async_mailbox_t<void(typename protocol_t::write_response_t)> resp_mailbox(
        cluster, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &resp_cond, _1));

    send(cluster, writeread_mailbox,
        w, ts, resp_mailbox.get_address());

    wait_any_t waiter(resp_cond.get_ready_signal(), interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t listener_read(
        mailbox_cluster_t *cluster,
        const typename listener_data_t<protocol_t>::read_mailbox_t::address_t &read_mailbox,
        typename protocol_t::read_t r, state_timestamp_t ts,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    promise_t<typename protocol_t::read_response_t> resp_cond;
    async_mailbox_t<void(typename protocol_t::read_response_t)> resp_mailbox(
        cluster, boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &resp_cond, _1));

    send(cluster, read_mailbox,
        r, ts, resp_mailbox.get_address());

    wait_any_t waiter(resp_cond.get_ready_signal(), interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t broadcaster_t<protocol_t>::read(typename protocol_t::read_t read, order_token_t order_token) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    state_timestamp_t timestamp;

    {
        mutex_acquisition_t mutex_acq(&mutex);
        pick_a_readable_dispatchee(&reader, &reader_lock);
        timestamp = current_timestamp;
        order_sink.check_out(order_token);
    }

    try {
        return listener_read<protocol_t>(cluster, reader->read_mailbox,
            read, timestamp,
            reader_lock.get_drain_signal());
    } catch (interrupted_exc_t) {
        throw mirror_lost_exc_t();
    }
}

template<class protocol_t>
typename protocol_t::write_response_t broadcaster_t<protocol_t>::write(typename protocol_t::write_t write, order_token_t order_token) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

    /* TODO: Make `target_ack_count` configurable */
    int target_ack_count = 1;

    /* We'll fill `write_wrapper` with the write that we start; we hold it in a
    shared pointer so that it stays alive while we check its `done_promise`. */
    boost::shared_ptr<incomplete_write_t> write_wrapper;

    typename protocol_t::write_response_t resp;

    {
        incomplete_write_ref_t write_ref;

        std::map<dispatchee_t *, auto_drainer_t::lock_t> writers;
        dispatchee_t *writereader;
        auto_drainer_t::lock_t writereader_lock;

        /* Set up the write */
        {
            /* We must hold the mutex while we are accessing
            `current_state_timestamp`, `incomplete_queue`, `dispatchees`, and
            `order_checkpoint`. */
            mutex_acquisition_t mutex_acq(&mutex);
            ASSERT_FINITE_CORO_WAITING;

            transition_timestamp_t timestamp = transition_timestamp_t::starting_from(current_timestamp);
            current_timestamp = timestamp.timestamp_after(); 
            order_sink.check_out(order_token);

            write_wrapper = boost::make_shared<incomplete_write_t>(
                this, write, timestamp, target_ack_count);
            incomplete_writes.push_back(write_wrapper);

            /* Create a reference so that `write` doesn't declare itself
            complete before we've even started */
            write_ref = incomplete_write_ref_t(write_wrapper);

            sanity_check(&mutex_acq);

            /* As long as we hold the lock, choose our writereader and take a
            snapshot of the dispatchee map */
            pick_a_readable_dispatchee(&writereader, &writereader_lock);
            writers = dispatchees;
        }

        /* Dispatch the write to all the dispatchees */

        /* First dispatch it to all the non-writereaders... */
        for (typename std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = writers.begin(); it != writers.end(); it++) {
            /* Make sure not to send a duplicate operation to `writereader` */
            if ((*it).first == writereader) continue;
            coro_t::spawn_sometime(boost::bind(&broadcaster_t::background_write, this,
                (*it).first, (*it).second, write_ref));
        }

        /* ... then dispatch it to the writereader */
        try {
            resp = listener_writeread<protocol_t>(cluster, writereader->writeread_mailbox,
                write_ref.get()->write, write_ref.get()->timestamp,
                writereader_lock.get_drain_signal());
        } catch (interrupted_exc_t) {
            throw mirror_lost_exc_t();
        }
        write_ref.get()->notify_acked();
    }

    /* Wait until `target_ack_count` has been reached, or until the write is
    declared impossible because there are too few mirrors. */
    bool success = write_wrapper->done_promise.wait();
    if (!success) throw insufficient_mirrors_exc_t();

    return resp;
}

template<class protocol_t>
broadcaster_t<protocol_t>::dispatchee_t::dispatchee_t(broadcaster_t *c, listener_data_t<protocol_t> d) THROWS_NOTHING :
    write_mailbox(d.write_mailbox), is_readable(false), controller(c),
    upgrade_mailbox(controller->cluster,
        boost::bind(&dispatchee_t::upgrade, this, _1, _2, auto_drainer_t::lock_t(&drainer))),
    downgrade_mailbox(controller->cluster,
        boost::bind(&dispatchee_t::downgrade, this, _1, auto_drainer_t::lock_t(&drainer)))
{
    controller->assert_thread();

    /* Grab mutex so no new writes start while we're setting ourselves up. */
    mutex_acquisition_t mutex_acq(&controller->mutex);
    ASSERT_FINITE_CORO_WAITING;

    controller->dispatchees[this] = auto_drainer_t::lock_t(&drainer);

    send(controller->cluster, d.intro_mailbox,
        controller->newest_complete_timestamp,
        upgrade_mailbox.get_address(),
        downgrade_mailbox.get_address());

    for (typename std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = controller->incomplete_writes.begin();
            it != controller->incomplete_writes.end(); it++) {
        coro_t::spawn_sometime(boost::bind(&broadcaster_t::background_write, controller,
            this, auto_drainer_t::lock_t(&drainer), incomplete_write_ref_t(*it)));
    }
}

template<class protocol_t>
broadcaster_t<protocol_t>::dispatchee_t::~dispatchee_t() THROWS_NOTHING {
    ASSERT_FINITE_CORO_WAITING;
    if (is_readable) controller->readable_dispatchees.remove(this);
    controller->dispatchees.erase(this);
    controller->assert_thread();
}

template<class protocol_t>
void broadcaster_t<protocol_t>::dispatchee_t::upgrade(
        typename listener_data_t<protocol_t>::writeread_mailbox_t::address_t wrm,
        typename listener_data_t<protocol_t>::read_mailbox_t::address_t rm,
        auto_drainer_t::lock_t)
        THROWS_NOTHING
{
    ASSERT_FINITE_CORO_WAITING;
    rassert(!is_readable);
    is_readable = true;
    writeread_mailbox = wrm;
    read_mailbox = rm;
    controller->readable_dispatchees.push_back(this);
}

template<class protocol_t>
void broadcaster_t<protocol_t>::dispatchee_t::downgrade(
        async_mailbox_t<void()>::address_t ack_addr,
        auto_drainer_t::lock_t)
        THROWS_NOTHING
{
    {
        ASSERT_FINITE_CORO_WAITING;
        rassert(is_readable);
        is_readable = false;
        controller->readable_dispatchees.remove(this);
    }
    if (!ack_addr.is_nil()) {
        send(controller->cluster, ack_addr);
    }
}

template<class protocol_t>
void broadcaster_t<protocol_t>::pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t) {

    ASSERT_FINITE_CORO_WAITING;

    if (readable_dispatchees.empty()) throw insufficient_mirrors_exc_t();
    *dispatchee_out = readable_dispatchees.head();

    /* Cycle the readable dispatchees so that the load gets distributed
    evenly */
    readable_dispatchees.pop_front();
    readable_dispatchees.push_back(*dispatchee_out);

    *lock_out = dispatchees[*dispatchee_out];
}

template<class protocol_t>
void broadcaster_t<protocol_t>::background_write(dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock, incomplete_write_ref_t write_ref) THROWS_NOTHING {
    try {
        listener_write<protocol_t>(cluster, mirror->write_mailbox,
            write_ref.get()->write, write_ref.get()->timestamp,
            mirror_lock.get_drain_signal());
    } catch (interrupted_exc_t) {
        return;
    }
    write_ref.get()->notify_acked();
}

template<class protocol_t>
void broadcaster_t<protocol_t>::end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING {
    mutex_acquisition_t mutex_acq(&mutex);
    ASSERT_FINITE_CORO_WAITING;
    /* It's safe to remove a write from the queue once it has acquired the root
    of every mirror's btree. We aren't notified when it acquires the root; we're
    notified when it completes, which happens some unspecified amount of time
    after it acquires the root. When a given write has finished on every mirror,
    then we know that it and every write before it have acquired the root, even
    though some of the writes before it might not have finished yet. So when a
    write is finished on every mirror, we remove it and every write before it
    from the queue. */
    while (newest_complete_timestamp < write->timestamp.timestamp_after()) {
        boost::shared_ptr<incomplete_write_t> removed_write = incomplete_writes.front();
        incomplete_writes.pop_front();
        rassert(newest_complete_timestamp == removed_write->timestamp.timestamp_before());
        newest_complete_timestamp = removed_write->timestamp.timestamp_after();
    }
    write->notify_no_more_acks();
    sanity_check(&mutex_acq);
}
