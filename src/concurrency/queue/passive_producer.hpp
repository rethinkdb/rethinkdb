#ifndef __CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP__
#define __CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP__

#include "errors.hpp"
#include "utils.hpp"
#include <boost/function.hpp>

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

class availability_t {
public:
    bool get() { return available; }
    void set_callback(boost::function<void()> fun) {
        rassert(!callback);
        rassert(fun);
        callback = fun;
    }
    void unset_callback() {
        rassert(callback);
        callback = 0;
    }
private:
    friend class availability_control_t;
    bool available;
    boost::function<void()> callback;
};

class availability_control_t : public availability_t {
public:
    availability_control_t() {
        available = false;
    }
    void set_available(bool a) {
        if (available != a) {
            available = a;
            if (callback) callback();
        }
    }
};

template<class value_t>
struct passive_producer_t {

    /* true if any data is available. `available` is const so that nobody
    screws around with it. */
    availability_t * const available;

    /* `pop()` removes a value from the producer and returns it. It is
    an error to call `pop()` when `available->get()` is `false`. */
    value_t pop() {
        rassert(available->get());
        return produce_next_value();
    }

protected:
    /* To implement a passive producer: Pass an `availability_t*` to the
    constructor. You can get it either by creating an `availability_control_t`,
    or by passing the `availability_t*` from some other queue. Override
    `produce_next_value()`; `produce_next_value()` will only be called when the
    `watchable_value_t<bool>` is true, but it must never fail or block. */

    passive_producer_t(availability_t *a) : available(a) { }
    virtual value_t produce_next_value() = 0;
    virtual ~passive_producer_t() { };

private:
    DISABLE_COPYING(passive_producer_t);
};

#endif /* __CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP__ */
