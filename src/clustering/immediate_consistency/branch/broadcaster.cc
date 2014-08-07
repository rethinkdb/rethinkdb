// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/broadcaster.hpp"

#include <functional>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "containers/death_runner.hpp"
#include "containers/uuid.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "logger.hpp"
#include "store_view.hpp"

/* Limits how many writes should be sent to a listener at once. */
const size_t DISPATCH_WRITES_CORO_POOL_SIZE = 64;

broadcaster_t::broadcaster_t(
        mailbox_manager_t *mm,
        rdb_context_t *_rdb_context,
        branch_history_manager_t *bhm,
        store_view_t *initial_svs,
        perfmon_collection_t *parent_perfmon_collection,
        order_source_t *order_source,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t)
    : broadcaster_collection(),
      broadcaster_membership(parent_perfmon_collection, &broadcaster_collection, "broadcaster"),
      rdb_context(_rdb_context),
      mailbox_manager(mm),
      branch_id(generate_uuid()),
      branch_history_manager(bhm),
      registrar(mailbox_manager, this)
{
    order_checkpoint.set_tagappend("broadcaster_t");

    /* Snapshot the starting point of the store; we'll need to record this
       and store it in the metadata. */
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    initial_svs->new_read_token(&read_token);

    region_map_t<binary_blob_t> origins_blob;
    initial_svs->do_get_metainfo(order_source->check_in("broadcaster_t(read)").with_read_mode(), &read_token, interruptor, &origins_blob);

    region_map_t<version_range_t> origins = to_version_range_map(origins_blob);

    /* Determine what the first timestamp of the new branch will be */
    state_timestamp_t initial_timestamp = state_timestamp_t::zero();

    typedef region_map_t<version_range_t> version_map_t;

    for (version_map_t::const_iterator it =  origins.begin();
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
        branch_birth_certificate_t birth_certificate;
        birth_certificate.region = initial_svs->get_region();
        birth_certificate.initial_timestamp = initial_timestamp;
        birth_certificate.origin = origins;

        cross_thread_signal_t ct_interruptor(interruptor, branch_history_manager->home_thread());
        on_thread_t th(branch_history_manager->home_thread());

        branch_history_manager->create_branch(branch_id, birth_certificate, &ct_interruptor);
    }

    /* Reset the store metadata. We should do this after making the branch
       entry in the global metadata so that we aren't left in a state where
       the store has been marked as belonging to a branch for which no
       information exists. */
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t> write_token;
    initial_svs->new_write_token(&write_token);
    initial_svs->set_metainfo(region_map_t<binary_blob_t>(initial_svs->get_region(),
                                                          binary_blob_t(version_range_t(version_t(branch_id, initial_timestamp)))),
                              order_source->check_in("broadcaster_t(write)"),
                              &write_token,
                              interruptor);

    /* Perform an initial sanity check. */
    sanity_check();

    /* Set `bootstrap_store` so that the initial listener can find it */
    bootstrap_svs = initial_svs;
}

branch_id_t broadcaster_t::get_branch_id() const {
    return branch_id;
}

broadcaster_business_card_t broadcaster_t::get_business_card() {
    branch_history_t branch_id_associated_branch_history;
    branch_history_manager->export_branch_history(branch_id, &branch_id_associated_branch_history);
    return broadcaster_business_card_t(branch_id, branch_id_associated_branch_history, registrar.get_business_card());
}

store_view_t *broadcaster_t::release_bootstrap_svs_for_listener() {
    guarantee(bootstrap_svs != NULL);
    store_view_t *tmp = bootstrap_svs;
    bootstrap_svs = NULL;
    return tmp;
}



/* `incomplete_write_t` represents a write that has been sent to some nodes
   but not completed yet. */
class broadcaster_t::incomplete_write_t : public home_thread_mixin_debug_only_t {
public:
    incomplete_write_t(broadcaster_t *p, const write_t &w, transition_timestamp_t ts, const ack_checker_t *ac, write_callback_t *cb) :
        write(w), timestamp(ts), ack_checker(ac), callback(cb), parent(p), incomplete_count(0) { }

    const write_t write;
    const transition_timestamp_t timestamp;
    const ack_checker_t *ack_checker;

    /* This is a callback to notify when the write has either succeeded or
    failed. Once the write succeeds, we will set this to `NULL` so that we
    don't call it again. */
    write_callback_t *callback;

    /* This is the set of peers that have acknowledged the write so far. When it satisfies
    the ack checker, then `callback->on_success()` will be called. */
    std::set<peer_id_t> ack_set;

private:
    friend class incomplete_write_ref_t;

    broadcaster_t *parent;
    int incomplete_count;

    DISABLE_COPYING(incomplete_write_t);
};

/* We keep track of which `incomplete_write_t`s have been acked by all the
   nodes using `incomplete_write_ref_t`. When there are zero
   `incomplete_write_ref_t`s for a given `incomplete_write_t`, then it is no
   longer incomplete. */

// TODO: make this noncopyable.
class broadcaster_t::incomplete_write_ref_t {
public:
    incomplete_write_ref_t() { }
    explicit incomplete_write_ref_t(const boost::shared_ptr<incomplete_write_t> &w) : write(w) {
        guarantee(w);
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

class broadcaster_t::dispatchee_t : public intrusive_list_node_t<dispatchee_t> {
public:
    dispatchee_t(broadcaster_t *c, listener_business_card_t d) THROWS_NOTHING :
        write_mailbox(d.write_mailbox), is_readable(false),
        local_listener(NULL), listener_id(generate_uuid()),
        queue_count(),
        queue_count_membership(&c->broadcaster_collection, &queue_count,
                               uuid_to_str(d.write_mailbox.get_peer().get_uuid())
                                           + "_broadcast_queue_count"),
        background_write_queue(&queue_count),
        background_write_workers(DISPATCH_WRITES_CORO_POOL_SIZE, &background_write_queue,
                                 &background_write_caller),
        controller(c),
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
        DEBUG_VAR mutex_assertion_t::acq_t acq(&controller->mutex);
        ASSERT_FINITE_CORO_WAITING;

        controller->dispatchees[this] = auto_drainer_t::lock_t(&drainer);

        /* This coroutine will send an intro message to the newly-registered
        listener. It needs to be a separate coroutine so that we don't block while
        holding `controller->mutex`. */
        coro_t::spawn_sometime(boost::bind(&dispatchee_t::send_intro, this,
            d, controller->newest_complete_timestamp, auto_drainer_t::lock_t(&drainer)));

        for (std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = controller->incomplete_writes.begin();
                it != controller->incomplete_writes.end(); it++) {

            coro_t::spawn_sometime(boost::bind(&broadcaster_t::background_write, controller,
                                               this, auto_drainer_t::lock_t(&drainer),
                                               incomplete_write_ref_t(*it),
                                               order_source.check_in("dispatchee_t"),
                                               fifo_source.enter_write()));
        }
    }

    ~dispatchee_t() THROWS_NOTHING {
        DEBUG_VAR mutex_assertion_t::acq_t acq(&controller->mutex);
        ASSERT_FINITE_CORO_WAITING;
        if (is_readable) controller->readable_dispatchees.remove(this);
        controller->refresh_readable_dispatchees_as_set();
        controller->dispatchees.erase(this);
        controller->assert_thread();
    }

    peer_id_t get_peer() {
        return write_mailbox.get_peer();
    }

private:
    /* The constructor spawns `send_intro()` in the background. */
    void send_intro(listener_business_card_t to_send_intro_to,
                    state_timestamp_t intro_timestamp,
                    auto_drainer_t::lock_t keepalive)
            THROWS_NOTHING {
        keepalive.assert_is_holding(&drainer);

        send(controller->mailbox_manager, to_send_intro_to.intro_mailbox,
             listener_intro_t(intro_timestamp,
                              upgrade_mailbox.get_address(),
                              downgrade_mailbox.get_address(),
                              listener_id));
    }

    /* `upgrade()` and `downgrade()` are mailbox callbacks. */
    void upgrade(listener_business_card_t::writeread_mailbox_t::address_t wrm,
                 listener_business_card_t::read_mailbox_t::address_t rm,
                 auto_drainer_t::lock_t)
            THROWS_NOTHING {
        DEBUG_VAR mutex_assertion_t::acq_t acq(&controller->mutex);
        ASSERT_FINITE_CORO_WAITING;
        guarantee(!is_readable);
        is_readable = true;
        writeread_mailbox = wrm;
        read_mailbox = rm;
        controller->readable_dispatchees.push_back(this);
        controller->refresh_readable_dispatchees_as_set();
    }

    void downgrade(mailbox_addr_t<void()> ack_addr, auto_drainer_t::lock_t) THROWS_NOTHING {
        {
            DEBUG_VAR mutex_assertion_t::acq_t acq(&controller->mutex);
            ASSERT_FINITE_CORO_WAITING;
            guarantee(is_readable);
            is_readable = false;
            controller->readable_dispatchees.remove(this);
            controller->refresh_readable_dispatchees_as_set();
        }
        if (!ack_addr.is_nil()) {
            send(controller->mailbox_manager, ack_addr);
        }
    }

public:
    listener_business_card_t::write_mailbox_t::address_t write_mailbox;
    bool is_readable;
    listener_business_card_t::writeread_mailbox_t::address_t writeread_mailbox;
    listener_business_card_t::read_mailbox_t::address_t read_mailbox;

    /* `local_listener` can be non-NULL if the dispatchee is local on this node
    (and on the same thread). */
    listener_t *local_listener;
    auto_drainer_t::lock_t local_listener_keepalive;

    /* This is used to enforce that operations are performed on the
       destination machine in the same order that we send them, even if the
       network layer reorders the messages. */
    fifo_enforcer_source_t fifo_source;

    uuid_u listener_id;

    // Accompanies the fifo_source.  It is questionable that we have a
    // separate order source just for the background writes.  What
    // about other writes that could interact with the background
    // writes?
    // TODO: Is something wrong with the ordering guarantees between background writes and other writes?
    order_source_t order_source;

    perfmon_counter_t queue_count;
    perfmon_membership_t queue_count_membership;
    unlimited_fifo_queue_t<std::function<void()> > background_write_queue;
    calling_callback_t background_write_caller;

private:
    coro_pool_t<std::function<void()> > background_write_workers;
    broadcaster_t *controller;
    auto_drainer_t drainer;

    listener_business_card_t::upgrade_mailbox_t upgrade_mailbox;
    listener_business_card_t::downgrade_mailbox_t downgrade_mailbox;

    DISABLE_COPYING(dispatchee_t);
};

void broadcaster_t::register_local_listener(
        const uuid_u &listener_id,
        listener_t *listener,
        auto_drainer_t::lock_t listener_keepalive) {
    for (auto it = dispatchees.begin(); it != dispatchees.end(); ++it) {
        if (it->first->listener_id == listener_id) {
            it->first->local_listener = listener;
            it->first->local_listener_keepalive = listener_keepalive;
            return;
        }
    }
    logERR("Non-critical error: Could not install local listener. "
           "You may experience reduced query performance.");
}

/* Functions to send a read or write to a mirror and wait for a response.
Important: These functions must send the message before responding to
`interruptor` being pulsed. */

void broadcaster_t::listener_write(
        broadcaster_t::dispatchee_t *mirror,
        const write_t &w, transition_timestamp_t ts,
        order_token_t order_token, fifo_enforcer_write_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    if (mirror->local_listener != NULL) {
        mirror->local_listener->local_write(w, ts, order_token, token, interruptor);
    } else {
        cond_t ack_cond;
        mailbox_t<void()> ack_mailbox(
            mailbox_manager,
            std::bind(&cond_t::pulse, &ack_cond));

        send(mailbox_manager, mirror->write_mailbox,
             w, ts, order_token, token, ack_mailbox.get_address());

        wait_interruptible(&ack_cond, interruptor);
    }
}

template <class response_t>
void store_listener_response(response_t *result_out, const response_t &result_in, cond_t *done) {
    *result_out = result_in;
    done->pulse();
}

void broadcaster_t::listener_read(
        broadcaster_t::dispatchee_t *mirror,
        const read_t &r, read_response_t *response, state_timestamp_t ts,
        order_token_t order_token, fifo_enforcer_read_token_t token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t)
{
    if (mirror->local_listener != NULL) {
        *response = mirror->local_listener->local_read(r, ts, order_token, token,
                                                       interruptor);
    } else {
        cond_t resp_cond;
        mailbox_t<void(read_response_t)> resp_mailbox(
            mailbox_manager,
            std::bind(&store_listener_response<read_response_t>, response, ph::_1, &resp_cond));

        send(mailbox_manager, mirror->read_mailbox,
             r, ts, order_token, token, resp_mailbox.get_address());

        wait_interruptible(&resp_cond, interruptor);
    }
}

void broadcaster_t::read(
        const read_t &read, read_response_t *response,
        fifo_enforcer_sink_t::exit_read_t *lock, order_token_t order_token,
        signal_t *interruptor)
        THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
    if (read.all_read()) {
        all_read(read, response, lock, order_token, interruptor);
    } else {
        single_read(read, response, lock, order_token, interruptor);
    }
}

void broadcaster_t::spawn_write(const write_t &write,
                                fifo_enforcer_sink_t::exit_write_t *lock,
                                order_token_t order_token,
                                write_callback_t *cb,
                                signal_t *interruptor,
                                const ack_checker_t *ack_checker) THROWS_ONLY(interrupted_exc_t) {

    rassert(cb != NULL);

    order_token.assert_write_mode();

    wait_interruptible(lock, interruptor);
    ASSERT_FINITE_CORO_WAITING;

    sanity_check();

    /* We have to be careful about the case where dispatchees are joining or
    leaving at the same time as we are doing the write. The way we handle
    this is via `mutex`. If the write reaches `mutex` before a new
    dispatchee does, then the new dispatchee's constructor will send off the
    write. Otherwise, the write will be sent directly to the new dispatchee
    by the loop further down in this very function. */
    DEBUG_VAR mutex_assertion_t::acq_t mutex_acq(&mutex);

    lock->end();

    /* If there are few enough readable dispatchees that the ack checker can't possibly be
    satisfied, then bail out early */
    if (!ack_checker->is_acceptable_ack_set(readable_dispatchees_as_set)) {
        cb->on_failure(false);
        return;
    }

    transition_timestamp_t timestamp = transition_timestamp_t::starting_from(current_timestamp);
    current_timestamp = timestamp.timestamp_after();
    order_token = order_checkpoint.check_through(order_token);

    boost::shared_ptr<incomplete_write_t> write_wrapper = boost::make_shared<incomplete_write_t>(
        this, write, timestamp, ack_checker, cb);
    incomplete_writes.push_back(write_wrapper);

    // You can't reuse the same callback for two writes.
    guarantee(cb->write == NULL);

    cb->write = write_wrapper.get();

    /* Create a reference so that `write` doesn't declare itself
    complete before we've even started */
    incomplete_write_ref_t write_ref = incomplete_write_ref_t(write_wrapper);

    /* As long as we hold the lock, take a snapshot of the dispatchee map
    and grab order tokens */
    for (std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
         it != dispatchees.end(); ++it) {
        /* Once we call `enter_write()`, we have committed to sending
        the write to every dispatchee. In particular, it's important
        that we don't check `interruptor` until the write is on its way
        to every dispatchee. */
        fifo_enforcer_write_token_t fifo_enforcer_token = it->first->fifo_source.enter_write();
        if (it->first->is_readable) {
            durability_requirement_t durability_requirement = write.durability();
            write_durability_t durability;
            switch (durability_requirement) {
            case DURABILITY_REQUIREMENT_DEFAULT:
                durability = ack_checker->get_write_durability(it->first->get_peer());
                break;
            case DURABILITY_REQUIREMENT_SOFT:
                durability = write_durability_t::SOFT;
                break;
            case DURABILITY_REQUIREMENT_HARD:
                durability = write_durability_t::HARD;
                break;
            default:
                unreachable();
            }

            it->first->background_write_queue.push(boost::bind(&broadcaster_t::background_writeread, this,
                it->first, it->second, write_ref, order_token, fifo_enforcer_token, durability));
        } else {
            it->first->background_write_queue.push(boost::bind(&broadcaster_t::background_write, this,
                it->first, it->second, write_ref, order_token, fifo_enforcer_token));
        }
    }
}

void broadcaster_t::pick_a_readable_dispatchee(
        dispatchee_t **dispatchee_out, mutex_assertion_t::acq_t *proof,
        auto_drainer_t::lock_t *lock_out)
        THROWS_ONLY(cannot_perform_query_exc_t) {
    ASSERT_FINITE_CORO_WAITING;
    proof->assert_is_holding(&mutex);

    if (readable_dispatchees.empty()) {
        throw cannot_perform_query_exc_t("No mirrors readable. this is strange because "
            "the primary mirror should be always readable.");
    }

    /* Prefer a local dispatchee (at the moment there always should be exactly one) */
    dispatchee_t *selected_dispatchee = NULL;
    for (dispatchee_t *d = readable_dispatchees.head();
         d != NULL;
         d = readable_dispatchees.next(d)) {
        const bool is_local =
            d->get_peer() == mailbox_manager->get_connectivity_cluster()->get_me();
        if (is_local) {
            selected_dispatchee = d;
            break;
        }
    }
    if (selected_dispatchee == NULL) {
        /* If we don't have a local one, just pick the first one we can get */
        selected_dispatchee = readable_dispatchees.head();
    }

    *dispatchee_out = selected_dispatchee;
    *lock_out = dispatchees[selected_dispatchee];
}

void broadcaster_t::get_all_readable_dispatchees(
        std::vector<dispatchee_t *> *dispatchees_out, mutex_assertion_t::acq_t *proof,
        std::vector<auto_drainer_t::lock_t> *locks_out)
        THROWS_ONLY(cannot_perform_query_exc_t) {
    ASSERT_FINITE_CORO_WAITING;
    proof->assert_is_holding(&mutex);
    if (readable_dispatchees.empty()) {
        throw cannot_perform_query_exc_t("No mirrors readable. this is strange because "
            "the primary mirror should be always readable.");
    }

    dispatchee_t *dispatchee = readable_dispatchees.head();

    while (dispatchee) {
        dispatchees_out->push_back(dispatchee);
        locks_out->push_back(dispatchees[dispatchee]);
        dispatchee = readable_dispatchees.next(dispatchee);
    }
}

void broadcaster_t::background_write(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token)
        THROWS_NOTHING {
    try {
        listener_write(mirror, write_ref.get()->write, write_ref.get()->timestamp,
                       order_token, token, mirror_lock.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        return;
    }
}

void broadcaster_t::background_writeread(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token, const write_durability_t durability)
        THROWS_NOTHING {
    try {
        write_response_t response;
        if (mirror->local_listener != NULL) {
            response = mirror->local_listener->local_writeread(
                    write_ref.get()->write, write_ref.get()->timestamp, order_token,
                    token, durability, mirror_lock.get_drain_signal());
        } else {
            cond_t response_cond;
            mailbox_t<void(write_response_t)> response_mailbox(
                mailbox_manager,
                boost::bind(&store_listener_response<write_response_t>, &response, _1,
                            &response_cond));

            send(mailbox_manager, mirror->writeread_mailbox, write_ref.get()->write,
                 write_ref.get()->timestamp, order_token, token,
                 response_mailbox.get_address(), durability);

            wait_interruptible(&response_cond, mirror_lock.get_drain_signal());
        }

        write_ref.get()->ack_set.insert(mirror->get_peer());
        if (write_ref.get()->ack_checker->is_acceptable_ack_set(write_ref.get()->ack_set)) {
            /* We might get here multiple times, if `is_acceptable_ack_set()`
            returns `true` before all of the acks have come back. To avoid
            calling the callback multiple times, we set `callback` to `NULL`
            after the first time. This also signals `end_write()` not to call
            `on_failure()`. */
            if (write_ref.get()->callback != NULL) {
                guarantee(write_ref.get()->callback->write == write_ref.get().get());
                write_ref.get()->callback->write = NULL;
                write_ref.get()->callback->on_success(response);
                write_ref.get()->callback = NULL;
            }
        }

    } catch (const interrupted_exc_t &) {
        return;
    }
}

void broadcaster_t::end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING {
    /* Acquire `mutex` so that anything that holds `mutex` sees a consistent
    view of `newest_complete_timestamp` and the front of `incomplete_writes`.
    Specifically, this is important for newly-created dispatchees and for
    `sanity_check()`. */
    DEBUG_VAR mutex_assertion_t::acq_t mutex_acq(&mutex);
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
        guarantee(newest_complete_timestamp == removed_write->timestamp.timestamp_before());
        newest_complete_timestamp = removed_write->timestamp.timestamp_after();
    }
    /* `write->callback` could be `NULL` if we already called `on_success()` on
    it */
    if (write->callback != NULL) {
        guarantee(write->callback->write == write.get());
        write->callback->write = NULL;
        write->callback->on_failure(true);
    }
}

void broadcaster_t::single_read(
    const read_t &read,
    read_response_t *response,
    fifo_enforcer_sink_t::exit_read_t *lock, order_token_t order_token,
    signal_t *interruptor)
    THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t)
{
    guarantee(!read.all_read());
    order_token.assert_read_mode();

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    state_timestamp_t timestamp;
    fifo_enforcer_read_token_t enforcer_token;

    {
        wait_interruptible(lock, interruptor);
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        lock->end();

        pick_a_readable_dispatchee(&reader, &mutex_acq, &reader_lock);
        timestamp = current_timestamp;
        order_token = order_checkpoint.check_through(order_token);

        /* This is safe even if `interruptor` gets pulsed because nothing
        checks `interruptor` until after we have sent the message. */
        enforcer_token = reader->fifo_source.enter_read();
    }

    try {
        wait_any_t interruptor2(reader_lock.get_drain_signal(), interruptor);
        listener_read(reader, read, response, timestamp, order_token, enforcer_token,
                      &interruptor2);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            throw cannot_perform_query_exc_t("lost contact with mirror during read");
        }
    }
}

void broadcaster_t::all_read(
    const read_t &read,
    read_response_t *response,
    fifo_enforcer_sink_t::exit_read_t *lock, order_token_t order_token,
    signal_t *interruptor)
    THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t)
{
    guarantee(read.all_read());
    order_token.assert_read_mode();

    std::vector<dispatchee_t *> readers;
    std::vector<auto_drainer_t::lock_t> reader_locks;
    state_timestamp_t timestamp;
    std::vector<fifo_enforcer_read_token_t> enforcer_tokens;

    {
        wait_interruptible(lock, interruptor);
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        lock->end();

        get_all_readable_dispatchees(&readers, &mutex_acq, &reader_locks);
        timestamp = current_timestamp;
        order_token = order_checkpoint.check_through(order_token);

        /* This is safe even if `interruptor` gets pulsed because nothing
        checks `interruptor` until after we have sent the message. */
        for (auto it = readers.begin(); it != readers.end(); ++it) {
            enforcer_tokens.push_back((*it)->fifo_source.enter_read());
        }
        guarantee(readers.size() == reader_locks.size() &&
                  readers.size() == enforcer_tokens.size());
    }

    try {
        wait_any_t interruptor2(interruptor);
        for (auto it = reader_locks.begin(); it != reader_locks.end(); ++it) {
            interruptor2.add(it->get_drain_signal());
        }
        std::vector<read_response_t> responses;
        responses.resize(readers.size());
        for (size_t i = 0; i < readers.size(); ++i) {
            listener_read(readers[i], read, &responses[i], timestamp, order_token,
                          enforcer_tokens[i], &interruptor2);
        }

        read.unshard(responses.data(), responses.size(), response,
                     rdb_context, &interruptor2);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            throw cannot_perform_query_exc_t("lost contact with mirror during read");
        }
    }

}

void broadcaster_t::refresh_readable_dispatchees_as_set() {
    /* You might think that we should update `readable_dispatchees_as_set`
    incrementally instead of refreshing the entire thing each time. However,
    this is difficult because two dispatchees could hypothetically have the same
    peer ID. This won't happen in production, but it could happen in testing,
    and we'd like the code not to break if that occurs. Besides, this code only
    runs when a dispatchees are added or removed, so the performance cost is
    negligible. */
    readable_dispatchees_as_set.clear();
    dispatchee_t *dispatchee = readable_dispatchees.head();
    while (dispatchee != NULL) {
        readable_dispatchees_as_set.insert(dispatchee->get_peer());
        dispatchee = readable_dispatchees.next(dispatchee);
    }
}

/* This function sanity-checks `incomplete_writes`, `current_timestamp`,
and `newest_complete_timestamp`. It mostly exists as a form of executable
documentation. */
void broadcaster_t::sanity_check() {
#ifndef NDEBUG
    mutex_assertion_t::acq_t acq(&mutex);
    state_timestamp_t ts = newest_complete_timestamp;
    for (std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = incomplete_writes.begin();
         it != incomplete_writes.end(); it++) {
        rassert(ts == (*it)->timestamp.timestamp_before());
        ts = (*it)->timestamp.timestamp_after();
    }
    rassert(ts == current_timestamp);
#endif
}

broadcaster_t::write_callback_t::write_callback_t() : write(NULL) { }

broadcaster_t::write_callback_t::~write_callback_t() {
    if (write) {
        guarantee(write->callback == this);
        write->callback = NULL;
    }
}
