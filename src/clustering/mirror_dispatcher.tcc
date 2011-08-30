
/* Functions to send a read or write to a mirror and wait for a response. */

template<class protocol_t>
void mirror_data_write(mailbox_cluster_t *cluster, const mirror_data_t &mirror, typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t tok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    cond_t ack_cond;
    async_mailbox_t<void()> ack_mailbox(
        cluster, boost::bind(&cond_t::pulse, &ack_cond));

    send(cluster, mirror.write_mailbox,
        w, ts, tok, ack_mailbox.get_address());

    wait_any_t waiter(&ack_cond, interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw mirror_lost_exc_t();
    rassert(ack_cond.is_pulsed());
}

template<class protocol_t>
typename protocol_t::write_response_t mirror_data_writeread(mailbox_cluster_t *cluster, const mirror_data_t &mirror, typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t tok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    promise_t<typename protocol_t::write_response_t> resp_cond;
    async_mailbox_t<void(typename protocol_t::response_t)> resp_mailbox(
        cluster, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &resp_cond));

    send(cluster, mirror.writeread_mailbox,
        w, ts, tok, resp_mailbox.get_address());

    wait_any_t waiter(resp_cond.get_ready_signal(), interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    return resp_cond.get();
}

template<class protocol_t>
typename protocol_t::read_response_t read(mailbox_cluster_t *cluster, const mirror_data_t &mirror, typename protocol_t::read_t r, order_token_t tok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    promise_t<typename protocol_t::read_response_t> resp_cond;
    async_mailbox_t<void(typename protocol_t::response_t)> resp_mailbox(
        cluster, boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &resp_cond));

    send(cluster, mirror.read_mailbox,
        r, tok, resp_mailbox.get_address());

    wait_any_t waiter(resp_cond.get_ready_signal(), interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    return resp_cond.get();
}

template<class protocol_t>
typename protocol_t::read_response_t mirror_dispatcher_t<protocol_t>::read(typename protocol_t::read_t r, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    pick_a_readable_dispatchee(&reader, &reader_lock);
    return reader->read(r, tok, reader_lock);
}

template<class protocol_t>
typename protocol_t::write_response_t mirror_dispatcher_t<protocol_t>::write(typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t tok) {

    /* `write_ref` is to keep `write` from freeing itself before we're done
    with it */
    queued_write_t::ref_t write_ref;
    queued_write_t *write;

    dispatchee_t *writereader;
    auto_drainer_t::lock_t writereader_lock;

    /* TODO: Make `target_ack_count` configurable */
    int target_ack_count = 1;
    promise_t<bool> done_promise;

    {
        /* We want to make sure that we send the write to every dispatchee
        and put it into the write queue atomically, so that every dispatchee
        either gets it directly or gets it off the write queue. Putting a
        critical section here makes sure that everything but the call to
        `writeread()` gets done atomically. Unfortunately, there's nothing
        we can do to enforce that nothing else happens between the insertion
        into `write_queue` and the call to `writeread`. */
        ASSERT_FINITE_CORO_WAITING;

        pick_a_readable_dispatchee(&writereader, &writereader_lock);

        write = new queued_write_t(
            &write_queue, &write_ref,
            w, ts, tok,
            target_ack_count, &done_promise);

        for (std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
                it != dispatchees.end(); it++) {
            dispatchee_t *writer = (*it).first;
            auto_drainer_t::lock_t writer_lock = (*it).second;
            /* Make sure not to send a duplicate operation to `writereader`
            */
            if (writer != writereader) {
                writer->begin_write_in_background(write, queued_write_t::ref_t(write), writer_lock);
            }
        }
    }

    typename protocol_t::write_response_t resp = writereader->writeread(write, queued_write_t::ref_t(write), writereader_lock);

    /* Release our reference to `write` so that it can go away if everything
    else is done with it too. */
    write_ref = queued_write_t::ref_t();

    bool success = done_promise.wait();
    if (!success) throw insufficient_mirrors_exc_t();

    return resp;
}

private:
    /* The `registrar_t` constructs a `dispatchee_t` for every mirror that
    connects to us. */

    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {

    public:
template<class protocol_t>
mirror_dispatcher_t<protocol_t>::dispatchee_t::dispatchee_t(mirror_dispatcher_t *c, mirror_data_t d) THROWS_NOTHING :
    controller(c), is_readable(false)
{
    ASSERT_FINITE_CORO_WAITING;

    controller->dispatchees[this] = auto_drainer_t::lock_t(&drainer);
    update(d);

    for (queued_write_t *write = controller->write_queue.tail();
            write; write = controller->write_queue.next(write)) {
        begin_write_in_background(write, queued_write_t::ref_t(write),
            auto_drainer_t::lock_t(&drainer));
    }
}

template<class protocol_t>
mirror_dispatcher_t<protocol_t>::dispatchee_t::~dispatchee_t() THROWS_NOTHING {
    if (is_readable) controller->readable_dispatchees.remove(this);
    controller->dispatchees.erase(this);
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::update(mirror_data_t d) THROWS_NOTHING {
    data = d;
    if (is_readable) {
        /* It's illegal to become unreadable after becoming readable */
        rassert(!data.writeread_mailbox.is_nil());
        rassert(!data.read_mailbox.is_nil());
    } else if (!is_readable && !d.read_mailbox.is_nil()) {
        /* We're upgrading to be readable */
        rassert(!d.writeread_mailbox.is_nil());
        is_readable = true;
        controller->readable_dispatchees.push_back(this);
    }
}

template<class protocol_t>
typename protocol_t::write_response_t mirror_dispatcher_t<protocol_t>::dispatchee_t::writeread(queued_write_t *write, queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t) {
    typename protocol_t::write_response_t resp;
    try {
        resp = mirror_data_writeread(controller->cluster, data,
            write->write, write->timestamp, write->order_token,
            keepalive.get_drain_signal());
    } catch (interrupted_exc_t) {
        throw mirror_lost_exc_t();
    }
    /* TODO: Inform `write` that it's safely on a mirror */
    return resp;
}

template<class protocol_t>
typename protocol_t::read_response_t mirror_dispatcher_t<protocol_t>::dispatchee_t::read(typename protocol_t::read_t read, order_token_t order_token, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t) {
    try {
        return mirror_data_read(controller->cluster, data,
            read, order_token,
            keepalive.get_drain_signal());
    } catch (interrupted_exc_t) {
        throw mirror_lost_exc_t();
    }
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::begin_write_in_background(queued_write_t *write, queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    /* It's safe to allocate this directly on the heap because
    `coro_t::spawn_sometime()` should always succeed, and
    `write_in_background()` will free `our_place_in_line`. */
    coro_fifo_acq_t *our_place_in_line = new coro_fifo_acq_t(&background_write_fifo);
    coro_t::spawn_sometime(boost::bind(&dispatchee_t::write_in_background, write, write_ref, keepalive, our_place_in_line));
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::write_in_background(queued_write_t *write, queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive, coro_fifo_acq_t *our_place_in_line) THROWS_NOTHING {
    delete spot_in_line;
    try {
        mirror_data_write(controller->cluster, data,
            write->write, write->timestamp, write->order_token,
            keepalive.get_drain_signal());
    } catch (interrupted_exc_t) {
        return;
    }
    /* TODO: Inform `write` that it's safely on a mirror */
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t) {

    ASSERT_FINITE_CORO_WAITING;

    if (readable_dispatchees.empty()) throw insufficient_mirrors_exc_t();
    *dispatchee_out = readable_dispatchees.head();

    /* Cycle the readable dispatchees so that the load gets distributed
    evenly */
    readable_dispatchees.pop_front();
    readable_dispatchees.push_back(*dispatchee_out);

    *lock_out = dispatchees[*dispatchee_out];
}
