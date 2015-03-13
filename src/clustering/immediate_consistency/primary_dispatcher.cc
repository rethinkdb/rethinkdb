// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/primary_dispatcher.hpp"

primary_dispatcher_t::dispatchee_registration_t::dispatchee_registration_t(
        primary_dispatcher_t *_parent,
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

    /* Grab mutex so we don't race with writes that are starting or finishing. */
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    parent->dispatchees[this] = auto_drainer_t::lock_t(&drainer);
    *first_timestamp_out = parent->current_timestamp;
}

primary_dispatcher_t::dispatchee_registration_t::~dispatchee_registration_t() {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    if (is_readable) {
        parent->readable_dispatchees_as_set.apply_atomic_op(
            [&](std::set<server_id_t> *servers) {
                guarantee(servers->count(server_id) == 0);
                servers->erase(server_id);
                return true;
            });
    }
    parent->dispatchees.erase(this);
    parent->assert_thread();
}

primary_dispatcher_t::dispatchee_registration_t::make_readable() {
    DEBUG_VAR mutex_assertion_t::acq_t acq(&parent->mutex);
    ASSERT_FINITE_CORO_WAITING;
    guarantee(!is_readable);
    is_readable = true;
    parent->readable_dispatchees_as_set.apply_atomic_op(
        [&](std::set<server_id_t> *servers) {
            guarantee(servers->count(server_id) == 0);
            servers->insert(server_id);
            return true;
            });
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
        const region_map_t<version_t> &base_version)
    perfmon_membership(parent_perfmon_collection, &perfmon_collection, "broadcaster"),
    readable_dispatchees_as_set(std::set<server_id_t>())
{
    current_timestamp = state_timestamp_t::zero();
    for (const auto &pair : base_version) {
        current_timestamp = std::max(pair.second.timestamp, current_timestamp);
    }
    most_recent_acked_write_timestamp = current_timestamp;

    branch_id = generate_uuid();
    branch_bc.origin = base_version;
    branch_bc.region = base_version.get_domain();
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
    rassert(region_is_superset(branch_bc.region, read.get_region()));
    order_token.assert_read_mode();

    dispatchee_registration_t *dispatchee = nullptr;
    auto_drainer_t::lock_t dispatchee_lock;
    state_timestamp_t min_timestamp;

    {
        wait_interruptible(lock, interruptor);
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        lock->end();

        /* Prefer the dispatchee with the highest acknowledged write version (to reduce
        the risk that the read has to wait for a write). If multiple ones are equal, use
        priority as a tie-breaker. */
        std::pair<state_timestamp_t, double> best;
        for (const auto &pair : dispatchees) {
            dispatchee_t *d = pair.first;
            if (!d->is_readable) {
                continue;
            }
            if (dispatchee == nullptr ||
                    std::make_pair(d->latest_acked_write, d->priority) > best) {
                dispatchee = d;
                dispatchee_lock = pair.second;
                best = std::make_pair(d->latest_acked_write, d->priority);
            }
        }

        rassert(dispatchee != nullptr, "Primary replica should always be readable");
        if (dispatchee == nullptr) {
            throw cannot_perform_query_exc_t("No replicas are available for reading.");
        }

        order_token = order_checkpoint.check_through(order_token);
        min_timestamp = most_recent_acked_write_timestamp;
    }

    try {
        wait_any_t interruptor2(dispatchee_lock.get_drain_signal(), interruptor);
        dispatchee->dispatchee->do_read(read, token, &interruptor2, response_out);
    } catch (const interrupted_exc_t &) {
        if (interruptor->is_pulsed()) {
            throw;
        } else {
            throw cannot_perform_query_exc_t("lost contact with mirror during read");
        }
    }
}

void primary_dispatcher_t::spawn_write(
        const write_t &write,
        order_token_t order_token,
        write_callback_t *cb) {
    ASSERT_FINITE_CORO_WAITING;
    assert_thread();
    rassert(region_is_superset(branch_bc.region, write.get_region()));
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

    counted_t<incomplete_write_t> incomplete_write = make_counted<incomplete_write_t>(
        write,
        current_timestamp.next(),
        order_checkpoint.check_through(order_token),
        durability,
        cb);

    current_timestamp = current_timestamp.next();

    // You can't reuse the same callback for two writes.
    guarantee(cb->write == NULL);
    cb->write = write_wrapper.get();

    for (const auto &pair : dispatchees) {
        pair.first->background_write_queue.push(
            std::bind(&primary_dispatcher_t::background_write, this,
                pair.first, pair.second, incomplete_write));
    }
}

primary_dispatcher_t::incomplete_write_t::incomplete_write_t(
        const write_t &w, state_timestamp_t ts, order_token_t ot, write_callback_t *cb) :
    write(w), timestamp(ts), order_token(ot), callback(cb)
    { }

primary_dispatcher_t::incomplete_write_t::~incomplete_write_t() {
    if (callback != nullptr) {
        callback->on_end();
    }
}

void primary_dispatcher_t::background_write(
        dispatchee_registration_t *dispatchee,
        auto_drainer_t::lock_t dispatchee_lock,
        counted_t<incomplete_write_t> write) THROWS_NOTHING {
    try {
        if (dispatchee->is_readable) {
            write_response_t response;
            dispatchee->dispatchee->do_write_sync(
                write->write, write->timestamp, write->order_token, write->durability,
                dispatchee_lock.get_drain_signal(), &response);

            /* Update latest acked write on the distpatchee so we can route queries
            to the fastest replica and avoid blocking there. */
            dispatchee->latest_acked_write =
                std::max(dispatchee->latest_acked_write, write_ref.get()->timestamp);

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

