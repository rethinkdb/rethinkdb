// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/primary_query_router.hpp"

/* `incomplete_write_t` represents a write that has been sent to some nodes but not
completed yet. */
class primary_query_router_t::incomplete_write_t : public home_thread_mixin_debug_only_t {
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

primary_query_router_t::dispatchee_t::dispatchee_t(
        primary_query_router_t *p,
        const std::string &perfmon_name,
        state_timestamp_t *first_timestamp_out) :
    parent(p),
    is_readable(false),
    queue_counte_membership(
        &parent->broadcaster_collection,
        &queue_count,
        perfmon_name + "broadcast_queue_count"),
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
