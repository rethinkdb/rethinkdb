#include "clustering/immediate_consistency/branch/broadcaster.hpp"

#include "utils.hpp"
#include <boost/make_shared.hpp>

#include "concurrency/coro_fifo.hpp"
#include "containers/death_runner.hpp"
#include "containers/uuid.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"

template <class protocol_t>
broadcaster_t<protocol_t>::broadcaster_t(mailbox_manager_t *mm,
              boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history,
              multistore_ptr_t<protocol_t> *initial_svs,
              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t)
    : mailbox_manager(mm),
      branch_id(generate_uuid()),
      registrar(mailbox_manager, this) {

    order_checkpoint.set_tagappend("broadcaster_t");

    /* Snapshot the starting point of the store; we'll need to record this
       and store it in the metadata. */
    const int num_stores = initial_svs->num_stores();
    boost::scoped_array< boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> > read_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t>[num_stores]);
    initial_svs->new_read_tokens(read_tokens.get(), num_stores);

    region_map_t<protocol_t, version_range_t> origins = initial_svs->get_all_metainfos(read_tokens.get(), num_stores, interruptor);

    /* Determine what the first timestamp of the new branch will be */
    state_timestamp_t initial_timestamp = state_timestamp_t::zero();

    typedef region_map_t<protocol_t, version_range_t> version_map_t;

    for (typename version_map_t::const_iterator it =  origins.begin();
         it != origins.end();
         it++) {
        state_timestamp_t part_timestamp = it->second.latest.timestamp;
        if (part_timestamp > initial_timestamp) {
            initial_timestamp = part_timestamp;
        }
    }
    current_timestamp = newest_complete_timestamp = initial_timestamp;

    /* Make an entry for this branch in the global branch history
       semilattice */
    {
        branch_birth_certificate_t<protocol_t> our_metadata;
        our_metadata.region = initial_svs->get_multistore_joined_region();
        our_metadata.initial_timestamp = initial_timestamp;
        our_metadata.origin = origins;

        std::map<branch_id_t, branch_birth_certificate_t<protocol_t> > singleton;
        singleton[branch_id] = our_metadata;
        metadata_field(&branch_history_t<protocol_t>::branches, branch_history)->join(singleton);
    }

    /* Reset the store metadata. We should do this after making the branch
       entry in the global metadata so that we aren't left in a state where
       the store has been marked as belonging to a branch for which no
       information exists. */
    boost::scoped_array< boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[num_stores]);
    initial_svs->new_write_tokens(write_tokens.get(), num_stores);
    initial_svs->set_all_metainfos(region_map_t<protocol_t, binary_blob_t>(initial_svs->get_multistore_joined_region(),
                                                                           binary_blob_t(version_range_t(version_t(branch_id, initial_timestamp)))),
                                   write_tokens.get(), num_stores,
                                   interruptor);

    /* Perform an initial sanity check. */
    sanity_check();

    /* Set `bootstrap_store` so that the initial listener can find it */
    bootstrap_svs = initial_svs;
}

template <class protocol_t>
branch_id_t broadcaster_t<protocol_t>::get_branch_id() {
    return branch_id;
}

template <class protocol_t>
broadcaster_business_card_t<protocol_t> broadcaster_t<protocol_t>::get_business_card() {
    return broadcaster_business_card_t<protocol_t>(branch_id, registrar.get_business_card());
}



/* `incomplete_write_t` represents a write that has been sent to some nodes
   but not completed yet. */
template <class protocol_t>
class broadcaster_t<protocol_t>::incomplete_write_t : public home_thread_mixin_t {
public:
    incomplete_write_t(broadcaster_t *p,
                       typename protocol_t::write_t w, transition_timestamp_t ts,
                       ack_callback_t *cb) :
        write(w), timestamp(ts),
        parent(p), incomplete_count(0),
        ack_callback(cb)
    {
        rassert(ack_callback);
    }

    void notify_acked(peer_id_t peer, typename protocol_t::write_response_t _resp) {
        if (!resp) {
            resp = _resp;
        }
        if (ack_callback && !done_promise.get_ready_signal()->is_pulsed()) {
            if (ack_callback->on_ack(peer) && resp) {
                done_promise.pulse(true);
            }
        }
    }

    void notify_acked(peer_id_t peer) {
        if (ack_callback && !done_promise.get_ready_signal()->is_pulsed()) {
            if (ack_callback->on_ack(peer) && resp) {
                done_promise.pulse(true);
            }
        }
    }

    void notify_no_more_acks() {
        if (ack_callback && !done_promise.get_ready_signal()->is_pulsed()) {
            done_promise.pulse(false);
        }
    }

    /* This is called if `write()` gets interrupted and it's no longer
       safe to access `ack_callback`. If this is called, `done_promise`
       won't get pulsed unless it already is. */
    void dont_touch_ack_callback() {
        ack_callback = NULL;
    }

    typename protocol_t::write_t write;
    transition_timestamp_t timestamp;

    /* `done_promise` gets pulsed with `true` when `ack_callback` is
       satisfied, or with `false` if it will never be satisfied. */
    promise_t<bool> done_promise;

    typename protocol_t::write_response_t get_resp() {
        rassert(resp);
        return *resp;
    }

private:
    friend class incomplete_write_ref_t;

    broadcaster_t *parent;
    int incomplete_count;
    ack_callback_t *ack_callback;
    boost::optional<typename protocol_t::write_response_t> resp;

    DISABLE_COPYING(incomplete_write_t);
};

/* We keep track of which `incomplete_write_t`s have been acked by all the
   nodes using `incomplete_write_ref_t`. When there are zero
   `incomplete_write_ref_t`s for a given `incomplete_write_t`, then it is no
   longer incomplete. */

// TODO: make this noncopyable.
template <class protocol_t>
class broadcaster_t<protocol_t>::incomplete_write_ref_t {
public:
    incomplete_write_ref_t() { }
    explicit incomplete_write_ref_t(const boost::shared_ptr<incomplete_write_t> &w) : write(w) {
        rassert(w);
        w->incomplete_count++;
    }
    incomplete_write_ref_t(const incomplete_write_ref_t &r) : write(r.write) {
        if (r.write) {
            r.write->incomplete_count++;
        }
    }
    ~incomplete_write_ref_t() {
        if (write) {
            write->incomplete_count--;
            if (write->incomplete_count == 0) {
                write->parent->end_write(write);
            }
        }
    }
    incomplete_write_ref_t &operator=(const incomplete_write_ref_t &r) {
        if (r.write) {
            r.write->incomplete_count++;
        }
        if (write) {
            write->incomplete_count--;
            if (write->incomplete_count == 0) {
                write->parent->end_write(write);
            }
        }
        write = r.write;
        return *this;
    }
    boost::shared_ptr<incomplete_write_t> get() {
        return write;
    }
private:
    boost::shared_ptr<incomplete_write_t> write;
};


/* The `registrar_t` constructs a `dispatchee_t` for every mirror that
   connects to us. */

template <class protocol_t>
class broadcaster_t<protocol_t>::dispatchee_t : public intrusive_list_node_t<dispatchee_t> {
public:
    dispatchee_t(broadcaster_t *c, listener_business_card_t<protocol_t> d) THROWS_NOTHING;
    ~dispatchee_t() THROWS_NOTHING;

    peer_id_t get_peer() {
        return write_mailbox.get_peer();
    }

    typename listener_business_card_t<protocol_t>::write_mailbox_t::address_t write_mailbox;
    bool is_readable;
    typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t writeread_mailbox;
    typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t read_mailbox;

    /* This is used to enforce that operations are performed on the
       destination machine in the same order that we send them, even if the
       network layer reorders the messages. */
    fifo_enforcer_source_t fifo_source;

private:
    /* The constructor spawns `send_intro()` in the background. */
    void send_intro(
                    listener_business_card_t<protocol_t> to_send_intro_to,
                    state_timestamp_t intro_timestamp,
                    auto_drainer_t::lock_t)
        THROWS_NOTHING;

    /* `upgrade()` and `downgrade()` are mailbox callbacks. */
    void upgrade(
                 typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t,
                 typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t,
                 auto_drainer_t::lock_t)
        THROWS_NOTHING;
    void downgrade(mailbox_addr_t<void()>, auto_drainer_t::lock_t) THROWS_NOTHING;

    broadcaster_t *controller;
    auto_drainer_t drainer;

    typename listener_business_card_t<protocol_t>::upgrade_mailbox_t upgrade_mailbox;
    typename listener_business_card_t<protocol_t>::downgrade_mailbox_t downgrade_mailbox;
};



/* Functions to send a read or write to a mirror and wait for a response.
Important: These functions must send the message before responding to
`interruptor` being pulsed. */

template<class protocol_t>
void listener_write(
        mailbox_manager_t *mailbox_manager,
        const typename listener_business_card_t<protocol_t>::write_mailbox_t::address_t &write_mailbox,
        typename protocol_t::write_t w, transition_timestamp_t ts,
        order_token_t order_token, fifo_enforcer_write_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    cond_t ack_cond;
    mailbox_t<void()> ack_mailbox(
        mailbox_manager,
        boost::bind(&cond_t::pulse, &ack_cond),
        mailbox_callback_mode_inline);

    send(mailbox_manager, write_mailbox,
         w, ts, order_token, token, ack_mailbox.get_address());

    wait_interruptible(&ack_cond, interruptor);
}

template<class protocol_t>
typename protocol_t::write_response_t listener_writeread(
        mailbox_manager_t *mailbox_manager,
        const typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t &writeread_mailbox,
        typename protocol_t::write_t w, transition_timestamp_t ts,
        order_token_t order_token, fifo_enforcer_write_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    promise_t<typename protocol_t::write_response_t> resp_cond;
    mailbox_t<void(typename protocol_t::write_response_t)> resp_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &resp_cond, _1),
        mailbox_callback_mode_inline);

    send(mailbox_manager, writeread_mailbox,
         w, ts, order_token, token, resp_mailbox.get_address());

    wait_interruptible(resp_cond.get_ready_signal(), interruptor);

    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t listener_read(
        mailbox_manager_t *mailbox_manager,
        const typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t &read_mailbox,
        typename protocol_t::read_t r, state_timestamp_t ts,
        order_token_t order_token, fifo_enforcer_read_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    promise_t<typename protocol_t::read_response_t> resp_cond;
    mailbox_t<void(typename protocol_t::read_response_t)> resp_mailbox(
        mailbox_manager,
        boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &resp_cond, _1),
        mailbox_callback_mode_inline);

    send(mailbox_manager, read_mailbox,
         r, ts, order_token, token, resp_mailbox.get_address());

    wait_interruptible(resp_cond.get_ready_signal(), interruptor);

    return resp_cond.get_value();
}

template<class protocol_t>
typename protocol_t::read_response_t broadcaster_t<protocol_t>::read(typename protocol_t::read_t read, fifo_enforcer_sink_t::exit_read_t *lock, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    state_timestamp_t timestamp;
    fifo_enforcer_read_token_t enforcer_token;

    {
        wait_interruptible(lock, interruptor);
        // TODO: We shouldn't reset exit_read_t _after_ we've
        // acquired the mutex.  We should release it after we've
        // gotten in line.
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        lock->reset();

        pick_a_readable_dispatchee(&reader, &mutex_acq, &reader_lock);
        timestamp = current_timestamp;
        order_token = order_checkpoint.check_through(order_token);

        /* This is safe even if `interruptor` gets pulsed because nothing
        checks `interruptor` until after we have sent the message. */
        enforcer_token = reader->fifo_source.enter_read();
    }

    try {
        wait_any_t interruptor2(reader_lock.get_drain_signal(), interruptor);
        return listener_read<protocol_t>(mailbox_manager, reader->read_mailbox,
                                         read, timestamp, order_token, enforcer_token,
                                         &interruptor2);
    } catch (interrupted_exc_t) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            throw cannot_perform_query_exc_t("lost contact with mirror during read");
        }
    }
}

template<class protocol_t>
typename protocol_t::write_response_t broadcaster_t<protocol_t>::write(typename protocol_t::write_t write, fifo_enforcer_sink_t::exit_write_t *lock, ack_callback_t *cb, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {

    /* We'll fill `write_wrapper` with the write that we start; we hold it in a
    shared pointer so that it stays alive while we check its `done_promise`. */
    boost::shared_ptr<incomplete_write_t> write_wrapper;

    death_runner_t dont_let_write_touch_ack_callback_after_exception;

    typename protocol_t::write_response_t resp;

    {
        incomplete_write_ref_t write_ref;

        std::map<dispatchee_t *, auto_drainer_t::lock_t> writers;
        std::map<dispatchee_t *, fifo_enforcer_write_token_t> enforcer_tokens;

        /* Set up the write. We have to be careful about the case where
        dispatchees are joining or leaving at the same time as we are doing the
        write. The way we handle this is via `mutex`. If the write reaches
        `mutex` before a new dispatchee does, then the new dispatchee's
        constructor will send off the write. Otherwise, the write will be sent
        directly to the new dispatchee by the loop further down in this very
        function. */
        {
            wait_interruptible(lock, interruptor);

            /* We must hold the mutex so that we don't race with other writes
            that are starting or with new dispatchees that are joining. */
            mutex_assertion_t::acq_t mutex_acq(&mutex);

            lock->reset();

            ASSERT_FINITE_CORO_WAITING;

            transition_timestamp_t timestamp = transition_timestamp_t::starting_from(current_timestamp);
            current_timestamp = timestamp.timestamp_after();
            order_token = order_checkpoint.check_through(order_token);

            write_wrapper = boost::make_shared<incomplete_write_t>(
                this, write, timestamp, cb);
            incomplete_writes.push_back(write_wrapper);

            /* If there's an exception, this will make sure that the
            `incomplete_write_t` object doesn't try to access `ack_callback`
            after we exit. */
            dont_let_write_touch_ack_callback_after_exception.fun = boost::bind(
                &incomplete_write_t::dont_touch_ack_callback, write_wrapper.get());

            /* Create a reference so that `write` doesn't declare itself
            complete before we've even started */
            write_ref = incomplete_write_ref_t(write_wrapper);

            /* As long as we hold the lock, choose our writereader and take a
            snapshot of the dispatchee map */
            writers = dispatchees;
            for (typename std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
                    it != dispatchees.end(); it++) {
                /* Once we call `enter_write()`, we have committed to sending
                the write to every dispatchee. In particular, it's important
                that we don't check `interruptor` until the write is on its way
                to every dispatchee. */
                enforcer_tokens[(*it).first] = (*it).first->fifo_source.enter_write();
            }
        }

        sanity_check();

        /* First check that someone is going to be able to give us a response */
        if (readable_dispatchees.empty()) {
            throw cannot_perform_query_exc_t("no mirrors readable. this is strange because the primary mirror should be always readable.");
        }

        /* Dispatch the write to all the dispatchees */
        for (typename std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = writers.begin(); it != writers.end(); it++) {
            coro_t::spawn_sometime(boost::bind(&broadcaster_t::background_write, this,
                                               (*it).first, (*it).second, write_ref, order_token, enforcer_tokens[(*it).first]));
        }
    }

    wait_interruptible(write_wrapper->done_promise.get_ready_signal(), interruptor);
    if (!write_wrapper->done_promise.get_value()) {
        throw cannot_perform_query_exc_t("insufficient mirrors to meet desired ack count");
    }

    return write_wrapper->get_resp();
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
    mutex_assertion_t::acq_t acq(&controller->mutex);
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
                                           this, auto_drainer_t::lock_t(&drainer), incomplete_write_ref_t(*it), order_token_t::ignore, fifo_source.enter_write()));
    }
}

template<class protocol_t>
broadcaster_t<protocol_t>::dispatchee_t::~dispatchee_t() THROWS_NOTHING {
    mutex_assertion_t::acq_t acq(&controller->mutex);
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
    mutex_assertion_t::acq_t acq(&controller->mutex);
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
        mutex_assertion_t::acq_t acq(&controller->mutex);
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
void broadcaster_t<protocol_t>::pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, mutex_assertion_t::acq_t *proof, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(cannot_perform_query_exc_t) {

    ASSERT_FINITE_CORO_WAITING;
    proof->assert_is_holding(&mutex);

    if (readable_dispatchees.empty()) {
        throw cannot_perform_query_exc_t("no mirrors readable. this is strange because the primary mirror should be always readable.");
    }
    *dispatchee_out = readable_dispatchees.head();

    /* Cycle the readable dispatchees so that the load gets distributed
    evenly */
    readable_dispatchees.pop_front();
    readable_dispatchees.push_back(*dispatchee_out);

    *lock_out = dispatchees[*dispatchee_out];
}

template<class protocol_t>
void broadcaster_t<protocol_t>::background_write(dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock, incomplete_write_ref_t write_ref, order_token_t order_token, fifo_enforcer_write_token_t token) THROWS_NOTHING {
    try {
        if (mirror->is_readable) {
            typename protocol_t::write_response_t resp = listener_writeread<protocol_t>(mailbox_manager, mirror->writeread_mailbox,
                                                                                        write_ref.get()->write, write_ref.get()->timestamp, order_token, token,
                                                                                        mirror_lock.get_drain_signal());

            write_ref.get()->notify_acked(mirror->get_peer(), resp);
        } else {
            listener_write<protocol_t>(mailbox_manager, mirror->write_mailbox,
                                       write_ref.get()->write, write_ref.get()->timestamp, order_token, token,
                                       mirror_lock.get_drain_signal());
            write_ref.get()->notify_acked(mirror->get_peer());
        }
    } catch (interrupted_exc_t) {
        return;
    }
}

template<class protocol_t>
void broadcaster_t<protocol_t>::end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING {
    /* Acquire `mutex` so that anything that holds `mutex` sees a consistent
    view of `newest_complete_timestamp` and the front of `incomplete_writes`.
    Specifically, this is important for newly-created dispatchees and for
    `sanity_check()`. */
    mutex_assertion_t::acq_t mutex_acq(&mutex);
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

    /* This function sanity-checks `incomplete_writes`, `current_timestamp`,
    and `newest_complete_timestamp`. It mostly exists as a form of executable
    documentation. */
template <class protocol_t>
void broadcaster_t<protocol_t>::sanity_check() {
#ifndef NDEBUG
    mutex_assertion_t::acq_t acq(&mutex);
    state_timestamp_t ts = newest_complete_timestamp;
    for (typename std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = incomplete_writes.begin();
         it != incomplete_writes.end(); it++) {
        rassert(ts == (*it)->timestamp.timestamp_before());
        ts = (*it)->timestamp.timestamp_after();
    }
    rassert(ts == current_timestamp);
#endif
}



#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class broadcaster_t<mock::dummy_protocol_t>;
template class broadcaster_t<memcached_protocol_t>;
