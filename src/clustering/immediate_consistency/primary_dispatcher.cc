// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/primary_dispatcher.hpp"

/* Limits how many writes should be sent to a dispatchee at once. */
const size_t DISPATCH_WRITES_CORO_POOL_SIZE = 64;

primary_dispatcher_t::dispatchee_registration_t::dispatchee_registration_t(
        primary_dispatcher_t *_parent,
        dispatchee_t *_dispatchee,
        const server_id_t &_server_id,
        double _priority,
        state_timestamp_t *first_timestamp_out) :
    parent(_parent),
    dispatchee(_dispatchee),
    server_id(_server_id),
    priority(_priority),
    is_ready(false),
    queue_count_membership(
        &parent->perfmon_collection,
        &queue_count,
        uuid_to_str(server_id) + "_broadcast_queue_count"),
    background_write_queue(&queue_count),
    background_write_workers(
        DISPATCH_WRITES_CORO_POOL_SIZE,
        &background_write_queue,
        &background_write_caller),
    latest_acked_write(state_timestamp_t::zero())
{
    parent->assert_thread();

    /* Grab mutex so we don't race with writes that are starting or finishing. */
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    parent->dispatchees[this] = auto_drainer_t::lock_t(&drainer);
    *first_timestamp_out = parent->current_timestamp;
}

primary_dispatcher_t::dispatchee_registration_t::~dispatchee_registration_t() {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    parent->assert_thread();
    parent->dispatchees.erase(this);
    if (is_ready) {
        parent->refresh_ready_dispatchees_as_set();
    }
}

void primary_dispatcher_t::dispatchee_registration_t::mark_ready() {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    guarantee(!is_ready);
    is_ready = true;
    parent->refresh_ready_dispatchees_as_set();
}

primary_dispatcher_t::write_callback_t::write_callback_t() : write(nullptr) { }

primary_dispatcher_t::write_callback_t::~write_callback_t() {
    if (write != nullptr) {
        guarantee(write->callback == this);
        write->callback = nullptr;
    }
}

primary_dispatcher_t::primary_dispatcher_t(
        perfmon_collection_t *parent_perfmon_collection,
        const region_map_t<version_t> &base_version) :
    perfmon_membership(parent_perfmon_collection, &perfmon_collection, "broadcaster"),
    ready_dispatchees_as_set(std::set<server_id_t>())
{
    current_timestamp = state_timestamp_t::zero();
    base_version.visit(base_version.get_domain(),
        [&](const region_t &, const version_t &v) {
            current_timestamp = std::max(v.timestamp, current_timestamp);
        });
    most_recent_acked_write_timestamp = current_timestamp;

    branch_id = generate_uuid();
    branch_bc.origin = base_version;
    branch_bc.initial_timestamp = current_timestamp;
}

void primary_dispatcher_t::read(
        const read_t &read,
        fifo_enforcer_sink_t::exit_read_t *lock,
        order_token_t order_token,
        signal_t *interruptor,
        read_response_t *response_out)
        THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
    assert_thread();
    rassert(region_is_superset(branch_bc.get_region(), read.get_region()));
    order_token.assert_read_mode();

    dispatchee_registration_t *dispatchee = nullptr;
    auto_drainer_t::lock_t dispatchee_lock;
    state_timestamp_t min_timestamp;

    {
        wait_interruptible(lock, interruptor);
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        lock->end();

        /* Prefer the dispatchee with the highest acknowledged write version
        (to reduce the risk that the read has to wait for a write). If multiple
        ones are equal, use priority as a tie-breaker. */
        std::pair<state_timestamp_t, double> best;
        for (const auto &pair : dispatchees) {
            dispatchee_registration_t *d = pair.first;
            if (!d->is_ready) {
                continue;
            }
            if (read.route_to_primary() && !d->dispatchee->is_primary()) {
                continue;
            }
            if (dispatchee == nullptr ||
                    std::make_pair(d->latest_acked_write, d->priority) > best) {
                dispatchee = d;
                dispatchee_lock = pair.second;
                best = std::make_pair(d->latest_acked_write, d->priority);
            }
        }

        rassert(dispatchee != nullptr, "Primary replica should always be ready");
        if (dispatchee == nullptr) {
            throw cannot_perform_query_exc_t(
                "No replicas are ready for reading.",
                query_state_t::FAILED);
        }

        order_checkpoint.check_through(order_token);
        min_timestamp = most_recent_acked_write_timestamp;
    }

    try {
        wait_any_t interruptor2(dispatchee_lock.get_drain_signal(), interruptor);
        dispatchee->dispatchee->do_read(read, min_timestamp, &interruptor2, response_out);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            throw cannot_perform_query_exc_t(
                "Lost contact with replica during read.",
                query_state_t::INDETERMINATE);
        }
    }
}

void primary_dispatcher_t::spawn_write(
        const write_t &write,
        order_token_t order_token,
        write_callback_t *cb) {
    ASSERT_FINITE_CORO_WAITING;
    assert_thread();
    rassert(region_is_superset(branch_bc.get_region(), write.get_region()));
    order_token.assert_write_mode();

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

    /* Assign a new timestamp to the write, unless it's a dummy write. */
    if (boost::get<dummy_write_t>(&write.write) == nullptr) {
        current_timestamp = current_timestamp.next();
    }

    counted_t<incomplete_write_t> incomplete_write = make_counted<incomplete_write_t>(
        write,
        current_timestamp,
        order_checkpoint.check_through(order_token),
        durability,
        cb);

    // You can't reuse the same callback for two writes.
    guarantee(cb->write == nullptr);
    cb->write = incomplete_write.get();

    for (const auto &pair : dispatchees) {
        pair.first->background_write_queue.push(
            std::bind(&primary_dispatcher_t::background_write, this,
                pair.first, pair.second, incomplete_write));
    }
}

primary_dispatcher_t::incomplete_write_t::incomplete_write_t(
        const write_t &w, state_timestamp_t ts, order_token_t ot,
        write_durability_t dur, write_callback_t *cb) :
    write(w), timestamp(ts), order_token(ot), durability(dur), callback(cb)
    { }

primary_dispatcher_t::incomplete_write_t::~incomplete_write_t() {
    if (callback != nullptr) {
        guarantee(callback->write == this);
        callback->write = nullptr;
        callback->on_end();
    }
}

void primary_dispatcher_t::background_write(
        dispatchee_registration_t *dispatchee,
        auto_drainer_t::lock_t dispatchee_lock,
        counted_t<incomplete_write_t> write) THROWS_NOTHING {
    try {
        /* Use a special path for dummy writes.
        dummy writes are used to check the table status, so we don't want to generate
        unnecessary disk writes or block on previous writes.
        We also never send a dummy write to a non-ready dispatchee, since
        there's no reason for queueing them up (they are no-ops anyway). */
        if (boost::get<dummy_write_t>(&write->write.write) != nullptr) {
            if (dispatchee->is_ready) {
                write_response_t response;
                dispatchee->dispatchee->do_dummy_write(
                    dispatchee_lock.get_drain_signal(), &response);
                if (write->callback != nullptr) {
                    guarantee(write->callback->write == write.get());
                    write->callback->on_ack(dispatchee->server_id, std::move(response));
                }
            }
            return;
        }

        if (dispatchee->is_ready) {
            write_response_t response;
            dispatchee->dispatchee->do_write_sync(
                write->write, write->timestamp, write->order_token, write->durability,
                dispatchee_lock.get_drain_signal(), &response);

            /* Update latest acked write on the distpatchee so we can route queries
            to the fastest replica and avoid blocking there. */
            dispatchee->latest_acked_write =
                std::max(dispatchee->latest_acked_write, write->timestamp);

            /* The write could potentially get acked when we call `on_ack()` on the
            callback. So make sure all reads started after this point will see this
            write. This is more conservative than necessary, since the write might not
            actually be acked to the client just because it was acked by this mirror; but
            this is the easiest thing to do. */
            most_recent_acked_write_timestamp
                = std::max(most_recent_acked_write_timestamp, write->timestamp);

            if (write->callback != nullptr) {
                guarantee(write->callback->write == write.get());
                write->callback->on_ack(dispatchee->server_id, std::move(response));
            }
        } else {
            dispatchee->dispatchee->do_write_async(
                write->write, write->timestamp, write->order_token,
                dispatchee_lock.get_drain_signal());
        }
    } catch (const interrupted_exc_t &) {
        /* ignore */
    }
}

void primary_dispatcher_t::refresh_ready_dispatchees_as_set() {
    /* Note that it's possible that we'll have multiple dispatchees with the same server
    ID. This won't happen during normal operation, but it can happen temporarily during
    a transition. For example, a secondary might disconnect and reconnect as a different
    dispatchee before we realize that its original dispatchee is no longer valid. */
    std::set<server_id_t> ready;
    for (const auto &pair : dispatchees) {
        if (pair.first->is_ready) {
            ready.insert(pair.first->server_id);
        }
    }
    ready_dispatchees_as_set.set_value(ready);
}

