// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/primary_query_router.hpp"

/* `incomplete_write_t` represents a write that has been sent to some nodes but not
completed yet. */
class primary_query_router_t::incomplete_write_t :
    public home_thread_mixin_debug_only_t
{
public:
    incomplete_write_t(primary_query_router_t *p,
                       const write_t &w,
                       state_timestamp_t ts,
                       write_callback_t *cb) :
        write(w), timestamp(ts), callback(cb), parent(p), incomplete_count(0) { }

    const write_t write;
    const state_timestamp_t timestamp;

    /* This is a callback to notify when the write has either succeeded or
    failed. Once the write succeeds, we will set this to `NULL` so that we
    don't call it again. */
    write_callback_t *callback;
private:
    friend class incomplete_write_ref_t;

    primary_query_router_t *parent;
    int incomplete_count;

    DISABLE_COPYING(incomplete_write_t);
};

/* We keep track of which `incomplete_write_t`s have been acked by all the nodes using
`incomplete_write_ref_t`. When there are zero `incomplete_write_ref_t`s for a given
`incomplete_write_t`, then it is no longer incomplete. */

// TODO: make this noncopyable.
class primary_query_router_t::incomplete_write_ref_t {
public:
    incomplete_write_ref_t() { }
    explicit incomplete_write_ref_t(
            const boost::shared_ptr<incomplete_write_t> &w) : write(w) {
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

primary_query_router_t::dispatchee_t::dispatchee_t(
        primary_query_router_t *_parent,
        replica_t *_replica,
        const server_id_t &_server_id,
        double _priority,
        state_timestamp_t *first_timestamp_out) :
    parent(_parent),
    replica(_replica),
    server_id(_server_id),
    priority(_priority),
    is_readable(false),
    queue_count_membership(
        &parent->broadcaster_collection,
        &queue_count,
        uuid_to_str(server_id) + "broadcast_queue_count"),
    background_write_queue(&queue_count),
    background_write_workers(
        DISPATCH_WRITES_CORO_POOL_SIZE,
        &background_write_queue,
        &background_write_caller),
    last_acked_write(state_timestamp_t::zero())
{
    parent->assert_thread();
    parent->sanity_check();

    /* Grab mutex so we don't race with writes that are starting or finishing. If we
    don't do this, bad things could happen: for example, a write might get dispatched to
    us twice if it starts after we're in `controller->dispatchees` but before we've
    iterated over `incomplete_writes`. */
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;

    parent->dispatchees[this] = auto_drainer_t::lock_t(&drainer);

    *first_timestamp_out = parent->newest_complete_timestamp;

    for (const auto &write : parent->incomplete_writes) {
        coro_t::spawn_sometime(
            std::bind(
                &primary_query_router_t::background_write,
                parent,
                this,
                auto_drainer_t::lock_t(&drainer),
                incomplete_write_ref_t(write),
                order_source.check_in("dispatchee_t"),
                fifo_source.enter_write()));
    }
}

primary_query_router_t::dispatchee_t::~dispatchee_t() {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    if (is_readable) {
        parent->readable_dispatchees.remove(this);
    }
    parent->refresh_readable_dispatchees_as_set();
    parent->dispatchees.erase(this);
    parent->assert_thread();
}

primary_query_router_t::dispatchee_t::set_readable(bool new_readable) {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    if (new_readable && !is_readable) {
        is_readable = true;
        parent->readable_dispatchees.push_back(this);
        parent->refresh_readable_dispatchees_as_set();
    } else if (!new_readable && is_readable) {
        is_readable = false;
        parent->readable_dispatchees.remove(this);
        controller->refresh_readable_dispatchees_as_set();
    }
}

primary_query_router_t::write_callback_t::write_callback_t() :
    write(NULL) { }

primary_query_router_t::write_callback_t::~write_callback_t() {
    if (write) {
        guarantee(write->callback == this);
        write->callback = NULL;
    }
}

primary_query_router_t::primary_query_router_t(
        perfmon_collection_t *parent_perfmon_collection,
        const region_map_t<version_t> &base_version)
    perfmon_membership(parent_perfmon_collection, &perfmon_collection, "broadcaster"),
    region(base_version.get_domain()),
    readable_dispatchees_as_set(std::set<server_id_t>())
{
    current_timestamp = state_timestamp_t::zero();
    for (const auto &pair : base_version) {
        current_timestamp = std::max(pair.second.timestamp, current_timestamp);
    }
    newest_complete_timestamp = most_recent_acked_write_timestamp = current_timestamp;
    sanity_check();

    branch_id = generate_uuid();
    branch_bc.origin = base_version;
    branch_bc.region = region;
    branch_bc.initial_timestamp = current_timestamp;
}

void primary_query_router_t::read(
        const read_t &read,
        read_response_t *response,
        fifo_enforcer_sink_t::exit_read_t *lock,
        order_token_t order_token,
        signal_t *interruptor)
        THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
    assert_thread();
    rassert(region_is_superset(branch_bc.region, read.get_region()));
    order_token.assert_read_mode();

    dispatchee_t *reader;
    auto_drainer_t::lock_t reader_lock;
    min_timestamp_token_t enforcer_token;

    {
        wait_interruptible(lock, interruptor);
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        lock->end();

        pick_a_readable_dispatchee(&reader, &mutex_acq, &reader_lock);
        order_token = order_checkpoint.check_through(order_token);

        /* Make sure the read runs *after* the most recent write that
        we did already acknowledge. */
        enforcer_token = min_timestamp_token_t(most_recent_acked_write_timestamp);
    }

    try {
        wait_any_t interruptor2(reader_lock.get_drain_signal(), interruptor);
        reader->replica->do_read(read, token, &interruptor2, response);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            throw cannot_perform_query_exc_t("lost contact with mirror during read");
        }
    }
}

void primary_query_router_t::spawn_write(
        const write_t &write,
        order_token_t order_token,
        write_callback_t *cb) {
    ASSERT_FINITE_CORO_WAITING;
    assert_thread();
    rassert(region_is_superset(branch_bc.region, write.get_region()));
    order_token.assert_write_mode();
    rassert(cb != NULL);

    sanity_check();

    /* We have to be careful about the case where dispatchees are joining or
    leaving at the same time as we are doing the write. The way we handle
    this is via `mutex`. If the write reaches `mutex` before a new
    dispatchee does, then the new dispatchee's constructor will send off the
    write. Otherwise, the write will be sent directly to the new dispatchee
    by the loop further down in this very function. */
    DEBUG_VAR mutex_assertion_t::acq_t mutex_acq(&mutex);

    write_durability_t durability;
    switch (write.durability()) {
        case DURABILITY_REQUIREMENT_DEFAULT:
            durability = cb->get_default_write_durability();
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

    state_timestamp_t timestamp = current_timestamp.next();
    current_timestamp = timestamp;
    order_token = order_checkpoint.check_through(order_token);

    boost::shared_ptr<incomplete_write_t> write_wrapper =
        boost::make_shared<incomplete_write_t>(
            this, write, timestamp, cb);
    incomplete_writes.push_back(write_wrapper);

    // You can't reuse the same callback for two writes.
    guarantee(cb->write == NULL);
    cb->write = write_wrapper.get();

    /* Create a reference so that `write` doesn't declare itself
    complete before we've even started */
    incomplete_write_ref_t write_ref = incomplete_write_ref_t(write_wrapper);

    /* As long as we hold the lock, grab order tokens for each dispatchee and put the
    writes into the queues */
    for (const auto &pair : dispatchees) {
        dispatchee_t *dispatchee = pair.first;
        /* Once we call `enter_write()`, we have committed to sending the write to every
        dispatchee. */
        fifo_enforcer_write_token_t fifo_token = dispatchee->fifo_source.enter_write();
        if (dispatchee->is_readable) {
            dispatchee->background_write_queue.push(
                std::bind(&primary_query_router_t::background_writeread, this,
                    dispatchee, pair.second, write_ref, order_token, fifo_token,
                    durability));
        } else {
            it->first->background_write_queue.push(
                std::bind(&broadcaster_t::background_write, this,
                    dispatchee, pair.second, write_ref, order_token, fifo_token));
        }
    }
}

void primary_query_router_t::pick_a_readable_dispatchee(
        dispatchee_t **dispatchee_out,
        mutex_assertion_t::acq_t *proof,
        auto_drainer_t::lock_t *lock_out)
        THROWS_ONLY(cannot_perform_query_exc_t) {
    ASSERT_FINITE_CORO_WAITING;
    proof->assert_is_holding(&mutex);

    if (readable_dispatchees.empty()) {
        throw cannot_perform_query_exc_t("No mirrors readable. this is strange because "
            "the primary replica mirror should be always readable.");
    }

    /* Prefer the dispatchee with the highest acknowledged write version (to reduce the
    risk that the read has to wait for a write). If multiple ones are equal, use priority
    as a tie-breaker. */
    dispatchee_t *best_dispatchee = nullptr;
    state_timestamp_t best_ts(state_timestamp_t::zero());
    double best_priority = 0;
    for (dispatchee_t *d = readable_dispatchees.head();
         d != NULL;
         d = readable_dispatchees.next(d)) {
        if (best_dispatchee == nullptr ||
                std::tie(d->latest_acked_write, d->priority) >
                    std::tie(best_ts, best_priority)) {
            best_dispatchee = d;
            best_ts = d->latest_acked_write;
            best_priority = d->priority;
        }
    }
    guarantee(best_dispatchee != nullptr);

    *dispatchee_out = best_dispatchee;
    *lock_out = dispatchees[best_dispatchee];
}

void primary_query_router_t::background_write(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token) THROWS_NOTHING {
    try {
        mirror->replica->do_write(
            write_ref.get()->write, write_ref.get()->timestamp, order_token, token,
            mirror_lock.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        return;
    }
}

void primary_query_router_t::background_writeread(
        dispatchee_t *mirror, auto_drainer_t::lock_t mirror_lock,
        incomplete_write_ref_t write_ref, order_token_t order_token,
        fifo_enforcer_write_token_t token, write_durability_t durability)
        THROWS_NOTHING {
    try {
        write_response_t response;
        mirrror->replica->do_writeread(
            write_ref.get()->write, write_ref.get()->timestamp, order_token, token,
            durability, mirror_lock.get_drain_signal();

        /* Update latest acked write on the distpatchee so we can route queries
        to the fastest replica and avoid blocking there. */
        mirror->latest_acked_write =
            std::max(mirror->latest_acked_write, write_ref.get()->timestamp);

        /* The write could potentially get acked when we call `on_ack()` on the callback.
        So make sure all reads started after this point will see this write. This is more
        conservative than necessary, since the write might not actually be acked to the
        client just because it was acked by this mirror; but this is the easiest thing to
        do. */
        most_recent_acked_write_timestamp
            = std::max(most_recent_acked_write_timestamp, write_ref.get()->timestamp);

        if (write_ref.get()->callback != nullptr) {
            guarantee(write_ref.get()->callback->write == write_ref.get().get());
            write_ref.get()->callback->on_ack(mirror->server_id, std::move(response));
        }
    } catch (const interrupted_exc_t &) {
        return;
    }
}

void primary_query_router_t::end_write(
        boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING {
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
    while (newest_complete_timestamp < write->timestamp) {
        boost::shared_ptr<incomplete_write_t> removed_write = incomplete_writes.front();
        incomplete_writes.pop_front();
        guarantee(newest_complete_timestamp.next() == removed_write->timestamp);
        newest_complete_timestamp = removed_write->timestamp;
    }
    if (write->callback != nullptr) {
        guarantee(write->callback->write == write.get());
        write->callback->write = NULL;
        write->callback->on_end();
    }
}

void primary_query_router_t::refresh_readable_dispatchees_as_set() {
    /* You might think that we should update `readable_dispatchees_as_set`
    incrementally instead of refreshing the entire thing each time. However,
    this is difficult because two dispatchees could hypothetically have the same
    server ID. This won't happen in production, but it could happen in testing,
    and we'd like the code not to break if that occurs. Besides, this code only
    runs when a dispatchees are added or removed, so the performance cost is
    negligible. */
    readable_dispatchees_as_set.apply_atomic_op(
        [&](std::set<server_id_t> *rds) {
            rds->clear();
            dispatchee_t *dispatchee = readable_dispatchees.head();
            while (dispatchee != NULL) {
                rds->insert(dispatchee->server_id);
                dispatchee = readable_dispatchees.next(dispatchee);
            }
            return true;
        });
}

void primary_query_router_t::sanity_check() {
#ifndef NDEBUG
    mutex_assertion_t::acq_t acq(&mutex);
    state_timestamp_t ts = newest_complete_timestamp;
    for (const auto &write : incomplete_writes) {
        rassert(ts.next() == write->timestamp);
        ts = write->timestamp;
    }
    rassert(ts == current_timestamp);
#endif
}

