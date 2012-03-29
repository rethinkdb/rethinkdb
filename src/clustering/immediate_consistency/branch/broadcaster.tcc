#include "concurrency/coro_fifo.hpp"

/* Functions to send a read or write to a mirror and wait for a response. */

template<class protocol_t>
void listener_write(
        mailbox_manager_t *mailbox_manager,
        const typename listener_business_card_t<protocol_t>::write_mailbox_t::address_t &write_mailbox,
        typename protocol_t::write_t w, transition_timestamp_t ts, fifo_enforcer_write_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    cond_t ack_cond;
    mailbox_t<void()> ack_mailbox(
        mailbox_manager, boost::bind(&cond_t::pulse, &ack_cond));

    send(mailbox_manager, write_mailbox,
        w, ts, token, ack_mailbox.get_address());

    wait_interruptible(&ack_cond, interruptor);
}

template<class protocol_t>
typename protocol_t::write_response_t listener_writeread(
        mailbox_manager_t *mailbox_manager,
        const typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t &writeread_mailbox,
        typename protocol_t::write_t w, transition_timestamp_t ts, fifo_enforcer_write_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    promise_t<typename protocol_t::write_response_t> resp_cond;
    mailbox_t<void(typename protocol_t::write_response_t)> resp_mailbox(
        mailbox_manager, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &resp_cond, _1));

    send(mailbox_manager, writeread_mailbox,
        w, ts, token, resp_mailbox.get_address());

    wait_interruptible(resp_cond.get_ready_signal(), interruptor);

    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t listener_read(
        mailbox_manager_t *mailbox_manager,
        const typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t &read_mailbox,
        typename protocol_t::read_t r, state_timestamp_t ts, fifo_enforcer_read_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    promise_t<typename protocol_t::read_response_t> resp_cond;
    mailbox_t<void(typename protocol_t::read_response_t)> resp_mailbox(
        mailbox_manager, boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &resp_cond, _1));

    send(mailbox_manager, read_mailbox,
        r, ts, token, resp_mailbox.get_address());

    wait_interruptible(resp_cond.get_ready_signal(), interruptor);

    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t broadcaster_t<protocol_t>::read(typename protocol_t::read_t read, fifo_enforcer_sink_t::exit_read_t *lock, order_token_t order_token) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    state_timestamp_t timestamp;
    fifo_enforcer_read_token_t enforcer_token;

    {
        lock->wait();
        // TODO: We shouldn't reset exit_read_t _after_ we've
        // acquired the mutex.  We should release it after we've
        // gotten in line.
        mutex_t::acq_t mutex_acq(&mutex);
        lock->reset();

        pick_a_readable_dispatchee(&reader, &mutex_acq, &reader_lock);
        timestamp = current_timestamp;
        order_sink.check_out(order_token);
        enforcer_token = reader->fifo_source.enter_read();
    }

    try {
        return listener_read<protocol_t>(mailbox_manager, reader->read_mailbox,
            read, timestamp, enforcer_token,
            reader_lock.get_drain_signal());
    } catch (interrupted_exc_t) {
        throw mirror_lost_exc_t();
    }
}

template<class protocol_t>
typename protocol_t::write_response_t broadcaster_t<protocol_t>::write(typename protocol_t::write_t write, UNUSED fifo_enforcer_sink_t::exit_write_t *lock, order_token_t order_token) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

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
        std::map<dispatchee_t *, fifo_enforcer_write_token_t> enforcer_tokens;

        /* Set up the write. We have to be careful about the case where
        dispatchees are joining or leaving at the same time as we are doing the
        write. The way we handle this is via `mutex`. If the write reaches
        `mutex` before a new dispatchee does, then the new dispatchee's
        constructor will send off the write. Otherwise, the write will be sent
        directly to the new dispatchee by the loop further down in this very
        function. */
        {

            lock->wait();

            // TODO: We shouldn't reset exit_read_t _after_ we've
            // acquired the mutex.  We should release it after we've
            // gotten in line.

            /* We must hold the mutex so that we don't race with other writes
            that are starting or with new dispatchees that are joining. */
            mutex_t::acq_t mutex_acq(&mutex);
            lock->reset();

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

            /* As long as we hold the lock, choose our writereader and take a
            snapshot of the dispatchee map */
            pick_a_readable_dispatchee(&writereader, &mutex_acq, &writereader_lock);
            writers = dispatchees;
            for (typename std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
                    it != dispatchees.end(); it++) {
                enforcer_tokens[(*it).first] = (*it).first->fifo_source.enter_write();
            }
        }

        sanity_check();

        /* Dispatch the write to all the dispatchees */

        /* First dispatch it to all the non-writereaders... */
        for (typename std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = writers.begin(); it != writers.end(); it++) {
            /* Make sure not to send a duplicate operation to `writereader` */
            if ((*it).first == writereader) continue;
            coro_t::spawn_sometime(boost::bind(&broadcaster_t::background_write, this,
                (*it).first, (*it).second, write_ref, enforcer_tokens[(*it).first]));
        }

        /* ... then dispatch it to the writereader */
        try {
            resp = listener_writeread<protocol_t>(mailbox_manager, writereader->writeread_mailbox,
                write_ref.get()->write, write_ref.get()->timestamp, enforcer_tokens[writereader],
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
broadcaster_t<protocol_t>::dispatchee_t::dispatchee_t(broadcaster_t *c, listener_business_card_t<protocol_t> d) THROWS_NOTHING :
    write_mailbox(d.write_mailbox), is_readable(false), controller(c),
    upgrade_mailbox(controller->mailbox_manager,
        boost::bind(&dispatchee_t::upgrade, this, _1, _2, auto_drainer_t::lock_t(&drainer))),
    downgrade_mailbox(controller->mailbox_manager,
        boost::bind(&dispatchee_t::downgrade, this, _1, auto_drainer_t::lock_t(&drainer)))
{
    controller->assert_thread();
    controller->sanity_check();

    /* Grab mutex so we don't race with writes that are starting or finishing.
    If we don't do this, bad things could happen: for example, a write might get
    dispatched to us twice if it starts after we're in `controller->dispatchees`
    but before we've iterated over `incomplete_writes`. */
    mutex_t::acq_t acq(&controller->mutex);
    ASSERT_FINITE_CORO_WAITING;

    controller->dispatchees[this] = auto_drainer_t::lock_t(&drainer);

    /* This coroutine will send an intro message to the newly-registered
    listener. It needs to be a separate coroutine so that we don't block while
    holding `controller->mutex`. */
    coro_t::spawn_sometime(boost::bind(&dispatchee_t::send_intro, this,
        d, controller->newest_complete_timestamp, auto_drainer_t::lock_t(&drainer)));

    for (typename std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = controller->incomplete_writes.begin();
            it != controller->incomplete_writes.end(); it++) {
        coro_t::spawn_sometime(boost::bind(&broadcaster_t::background_write, controller,
            this, auto_drainer_t::lock_t(&drainer), incomplete_write_ref_t(*it), fifo_source.enter_write()));
    }
}

template<class protocol_t>
broadcaster_t<protocol_t>::dispatchee_t::~dispatchee_t() THROWS_NOTHING {
    mutex_t::acq_t acq(&controller->mutex);
    ASSERT_FINITE_CORO_WAITING;
    if (is_readable) controller->readable_dispatchees.remove(this);
    controller->dispatchees.erase(this);
    controller->assert_thread();
}

template<class protocol_t>
void broadcaster_t<protocol_t>::dispatchee_t::send_intro(listener_business_card_t<protocol_t> to_send_intro_to, state_timestamp_t intro_timestamp, auto_drainer_t::lock_t lock) THROWS_NOTHING {
    lock.assert_is_holding(&drainer);
    send(controller->mailbox_manager, to_send_intro_to.intro_mailbox,
        intro_timestamp,
        upgrade_mailbox.get_address(),
        downgrade_mailbox.get_address()
        );
}

template<class protocol_t>
void broadcaster_t<protocol_t>::dispatchee_t::upgrade(
        typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t wrm,
        typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t rm,
        auto_drainer_t::lock_t)
        THROWS_NOTHING
{
    mutex_t::acq_t acq(&controller->mutex);
    ASSERT_FINITE_CORO_WAITING;
    rassert(!is_readable);
    is_readable = true;
    writeread_mailbox = wrm;
    read_mailbox = rm;
    controller->readable_dispatchees.push_back(this);
}

template<class protocol_t>
void broadcaster_t<protocol_t>::dispatchee_t::downgrade(mailbox_addr_t<void()> ack_addr, auto_drainer_t::lock_t) THROWS_NOTHING {
    {
        mutex_t::acq_t acq(&controller->mutex);
        ASSERT_FINITE_CORO_WAITING;
        rassert(is_readable);
        is_readable = false;
        controller->readable_dispatchees.remove(this);
    }
    if (!ack_addr.is_nil()) {
        send(controller->mailbox_manager, ack_addr);
    }
}

template<class protocol_t>
void broadcaster_t<protocol_t>::pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, mutex_t::acq_t *proof, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t) {

    ASSERT_FINITE_CORO_WAITING;
    proof->assert_is_holding(&mutex);

    if (readable_dispatchees.empty()) throw insufficient_mirrors_exc_t();
    *dispatchee_out = readable_dispatchees.head();

    /* Cycle the readable dispatchees so that the load gets distributed
    evenly */
    readable_dispatchees.pop_front();
    readable_dispatchees.push_back(*dispatchee_out);

    *lock_out = dispatchees[*dispatchee_out];
}

template<class protocol_t>
void broadcaster_t<protocol_t>::background_write(dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock, incomplete_write_ref_t write_ref, fifo_enforcer_write_token_t token) THROWS_NOTHING {
    try {
        listener_write<protocol_t>(mailbox_manager, mirror->write_mailbox,
            write_ref.get()->write, write_ref.get()->timestamp, token,
            mirror_lock.get_drain_signal());
    } catch (interrupted_exc_t) {
        return;
    }
    write_ref.get()->notify_acked();
}

template<class protocol_t>
void broadcaster_t<protocol_t>::end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING {
    /* Acquire `mutex` so that anything that holds `mutex` sees a consistent
    view of `newest_complete_timestamp` and the front of `incomplete_writes`.
    Specifically, this is important for newly-created dispatchees and for
    `sanity_check()`. */
    mutex_t::acq_t mutex_acq(&mutex);
    ASSERT_FINITE_CORO_WAITING;
    /* It's safe to remove a write from the queue once it has acquired the root
    of every mirror's btree. We aren't notified when it acquires the root; we're
    notified when it finishes, which happens some unspecified amount of time
    after it acquires the root. When a given write has finished on every mirror,
    then we know that it and every write before it have acquired the root, even
    though some of the writes before it might not have finished yet. So when a
    write is finished on every mirror, we remove it and every write before it
    from the queue. This loop makes one iteration on average for every call to
    `end_write()`, but it could make multiple iterations or zero iterations on
    any given call. */
    while (newest_complete_timestamp < write->timestamp.timestamp_after()) {
        boost::shared_ptr<incomplete_write_t> removed_write = incomplete_writes.front();
        incomplete_writes.pop_front();
        rassert(newest_complete_timestamp == removed_write->timestamp.timestamp_before());
        newest_complete_timestamp = removed_write->timestamp.timestamp_after();
    }
    write->notify_no_more_acks();
}
