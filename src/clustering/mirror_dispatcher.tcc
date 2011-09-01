#include "concurrency/coro_fifo.hpp"

/* Functions to send a read or write to a mirror and wait for a response. */

template<class protocol_t>
void mirror_data_write(mailbox_cluster_t *cluster, const typename mirror_dispatcher_metadata_t<protocol_t>::mirror_data_t &mirror, typename protocol_t::write_t w, transition_timestamp_t ts, order_token_t tok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    cond_t ack_cond;
    async_mailbox_t<void()> ack_mailbox(
        cluster, boost::bind(&cond_t::pulse, &ack_cond));

    send(cluster, mirror.write_mailbox,
        w, ts, tok, ack_mailbox.get_address());

    wait_any_t waiter(&ack_cond, interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    rassert(ack_cond.is_pulsed());
}

template<class protocol_t>
typename protocol_t::write_response_t mirror_data_writeread(mailbox_cluster_t *cluster, const typename mirror_dispatcher_metadata_t<protocol_t>::mirror_data_t &mirror, typename protocol_t::write_t w, transition_timestamp_t ts, order_token_t tok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    promise_t<typename protocol_t::write_response_t> resp_cond;
    async_mailbox_t<void(typename protocol_t::write_response_t)> resp_mailbox(
        cluster, boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &resp_cond, _1));

    send(cluster, mirror.writeread_mailbox,
        w, ts, tok, resp_mailbox.get_address());

    wait_any_t waiter(resp_cond.get_ready_signal(), interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t mirror_data_read(mailbox_cluster_t *cluster, const typename mirror_dispatcher_metadata_t<protocol_t>::mirror_data_t &mirror, typename protocol_t::read_t r, order_token_t tok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    promise_t<typename protocol_t::read_response_t> resp_cond;
    async_mailbox_t<void(typename protocol_t::read_response_t)> resp_mailbox(
        cluster, boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &resp_cond, _1));

    send(cluster, mirror.read_mailbox,
        r, tok, resp_mailbox.get_address());

    wait_any_t waiter(resp_cond.get_ready_signal(), interruptor);
    waiter.wait_lazily_unordered();

    if (interruptor->is_pulsed()) throw interrupted_exc_t();
    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t mirror_dispatcher_t<protocol_t>::read(typename protocol_t::read_t r, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    pick_a_readable_dispatchee(&reader, &reader_lock);
    return reader->read(r, tok, reader_lock);
}

template<class protocol_t>
typename protocol_t::write_response_t mirror_dispatcher_t<protocol_t>::write(typename protocol_t::write_t w, transition_timestamp_t ts, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

    /* TODO: Make `target_ack_count` configurable */
    int target_ack_count = 1;
    promise_t<bool> done_promise;

    typename protocol_t::write_response_t resp;

    {
        /* We must put the write on the queue and dispatch it to every
        dispatchee atomically, so that no new dispatchee is created between when
        we put it on the queue and when we send it to the dispatchees. Nothing
        in this scope is supposed to block except for the final call to
        `writeread()`. */

        /* `write_ref` is to keep the write from freeing itself before we're
        done with it. We put it in an inner scope so that we don't hold onto it
        any longer than we need to. */
        typename queued_write_t::ref_t write_ref =
            queued_write_t::spawn(&write_queue,
                w, ts, tok,
                target_ack_count, &done_promise);

        dispatchee_t *writereader;
        auto_drainer_t::lock_t writereader_lock;
        pick_a_readable_dispatchee(&writereader, &writereader_lock);

        for (typename std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
                it != dispatchees.end(); it++) {
            dispatchee_t *writer = (*it).first;
            auto_drainer_t::lock_t writer_lock = (*it).second;
            /* Make sure not to send a duplicate operation to `writereader` */
            if (writer != writereader) {
                writer->begin_write_in_background(write_ref, writer_lock);
            }
        }

        resp = writereader->writeread(write_ref, writereader_lock);
    }

    bool success = done_promise.wait();
    if (!success) throw insufficient_mirrors_exc_t();

    return resp;
}

template<class protocol_t>
mirror_dispatcher_t<protocol_t>::dispatchee_t::dispatchee_t(mirror_dispatcher_t *c, mirror_data_t d) THROWS_NOTHING :
    controller(c), is_readable(false)
{
    ASSERT_FINITE_CORO_WAITING;

    controller->assert_thread();

    controller->dispatchees[this] = auto_drainer_t::lock_t(&drainer);
    update(d);

    for (queued_write_t *write = controller->write_queue.tail();
            write; write = controller->write_queue.next(write)) {
        begin_write_in_background(
            typename queued_write_t::ref_t(write),
            auto_drainer_t::lock_t(&drainer));
    }
}

template<class protocol_t>
mirror_dispatcher_t<protocol_t>::dispatchee_t::~dispatchee_t() THROWS_NOTHING {
    ASSERT_FINITE_CORO_WAITING;
    if (is_readable) controller->readable_dispatchees.remove(this);
    controller->dispatchees.erase(this);
    controller->assert_thread();
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::update(mirror_data_t d) THROWS_NOTHING {
    ASSERT_FINITE_CORO_WAITING;
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
typename protocol_t::write_response_t mirror_dispatcher_t<protocol_t>::dispatchee_t::writeread(typename queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t) {
    keepalive.assert_is_holding(&drainer);
    typename protocol_t::write_response_t resp;
    try {
        resp = mirror_data_writeread<protocol_t>(controller->cluster, data,
            write_ref.get()->write, write_ref.get()->timestamp, write_ref.get()->order_token,
            keepalive.get_drain_signal());
    } catch (interrupted_exc_t) {
        throw mirror_lost_exc_t();
    }
    write_ref.get()->notify_acked();
    return resp;
}

template<class protocol_t>
typename protocol_t::read_response_t mirror_dispatcher_t<protocol_t>::dispatchee_t::read(typename protocol_t::read_t read, order_token_t order_token, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t) {
    keepalive.assert_is_holding(&drainer);
    try {
        return mirror_data_read<protocol_t>(controller->cluster, data,
            read, order_token,
            keepalive.get_drain_signal());
    } catch (interrupted_exc_t) {
        throw mirror_lost_exc_t();
    }
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::begin_write_in_background(typename queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    keepalive.assert_is_holding(&drainer);
    /* It's safe to allocate this directly on the heap because
    `coro_t::spawn_sometime()` should always succeed, and
    `write_in_background()` will free `our_place_in_line`. */
    coro_fifo_acq_t *our_place_in_line = new coro_fifo_acq_t;
    our_place_in_line->enter(&background_write_fifo);
    coro_t::spawn_sometime(boost::bind(&dispatchee_t::write_in_background, this, write_ref, keepalive, our_place_in_line));
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::dispatchee_t::write_in_background(typename queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive, coro_fifo_acq_t *our_place_in_line) THROWS_NOTHING {
    our_place_in_line->leave();
    delete our_place_in_line;
    try {
        mirror_data_write<protocol_t>(controller->cluster, data,
            write_ref.get()->write, write_ref.get()->timestamp, write_ref.get()->order_token,
            keepalive.get_drain_signal());
    } catch (interrupted_exc_t) {
        return;
    }
    write_ref.get()->notify_acked();
}

template<class protocol_t>
void mirror_dispatcher_t<protocol_t>::pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t) {

    ASSERT_FINITE_CORO_WAITING;

    if (readable_dispatchees.empty()) throw insufficient_mirrors_exc_t();
    *dispatchee_out = readable_dispatchees.head();

    /* Cycle the readable dispatchees so that the load gets distributed
    evenly */
    readable_dispatchees.pop_front();
    readable_dispatchees.push_back(*dispatchee_out);

    *lock_out = dispatchees[*dispatchee_out];
}
