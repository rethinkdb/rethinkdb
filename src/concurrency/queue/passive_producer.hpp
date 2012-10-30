// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP_
#define CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP_

#include "errors.hpp"
#include "arch/runtime/coroutines.hpp"

/* A passive producer is an object (often a queue) that a consumer can read from
by calling a method. It's called "passive" because the consumer calls a method
on the producer to ask for data, rather than the producer calling a method on
the consumer to give it data.

In general you must create the passive producer before you create the consumer,
and destroy the consumer before you destroy the passive producer.

`passive_producer_t` is not thread-safe.

`availability_t` is used to communicate to the consumer whether or not data is
available on the `passive_producer_t`. `availability_t` is separate from
`passive_producer_t` because often there are several `passive_producer_t`s in a
chain, where each one's availability is equal to the availability of the one
before it, and when `availability_t` is a separate object they can all share the
same `availability_t`. `availability_control_t` is a concrete subclass of
`availability_t` that can actually be instantiated. */

class availability_callback_t {
public:
    availability_callback_t() { }
    virtual void on_source_availability_changed() = 0;
protected:
    virtual ~availability_callback_t() { }
    DISABLE_COPYING(availability_callback_t);
};

class availability_t {
public:
    availability_t() : available(false), callback(NULL) { }
    bool get() { return available; }
    void set_callback(availability_callback_t *cb) {
        rassert(!callback);
        rassert(cb);
        callback = cb;
    }
    void unset_callback() {
        rassert(callback);
        callback = NULL;
    }
private:
    friend class availability_control_t;
    bool available;
    availability_callback_t *callback;

    DISABLE_COPYING(availability_t);
};

class availability_control_t : public availability_t {
public:
    availability_control_t() { }
    void set_available(bool a) {
        if (available != a) {
            available = a;
            if (callback) {
                callback->on_source_availability_changed();
            }
        }
    }

private:
    DISABLE_COPYING(availability_control_t);
};

template<class value_t>
struct passive_producer_t {

    /* true if any data is available. */
    availability_t *const available;

    /* `pop()` removes a value from the producer and returns it. It is
    an error to call `pop()` when `available->get()` is `false`. */
    value_t pop() {
        ASSERT_NO_CORO_WAITING;
        rassert(available->get());
        return produce_next_value();
    }

protected:
    /* To implement a passive producer: Pass an `availability_t*` to the
    constructor. You can get it either by creating an `availability_control_t`,
    or by passing the `availability_t*` from some other queue. Override
    `produce_next_value()`; `produce_next_value()` will only be called when the
    `watchable_value_t<bool>` is true, but it must never fail or block. */

    explicit passive_producer_t(availability_t *a) : available(a) { }
    virtual value_t produce_next_value() = 0;
    virtual ~passive_producer_t() { }

private:
    DISABLE_COPYING(passive_producer_t);
};

#endif /* CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP_ */
