#ifndef __CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP__
#define __CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP__

#include "concurrency/watchable_value.hpp"

/* A passive producer is an object (often a queue) that a consumer can read from
by calling a method. It's called "passive" because the consumer calls a method
on the producer to ask for data, rather than the producer calling a method on
the consumer to give it data.

In general you must create the passive producer before you create the consumer,
and destroy the consumer before you destroy the passive producer.

`passive_producer_t` is not thread-safe. */

template<class value_t>
struct passive_producer_t {

    /* true if any data is available. `available` is const so that nobody
    screws around with it. */
    watchable_value_t<bool> * const available;

    /* `pop()` removes a value from the producer and returns it. It is
    an error to call `pop()` when `available->get()` is `false`. */
    value_t pop() {
        rassert(available->get());
        return produce_next_value();
    }

protected:
    /* To implement a passive producer: Pass a `watchable_value_t<bool>*` to the
    constructor; the current value of this `watchable_value_t<bool>` will be
    taken to indicate whether data is available. Override
    `produce_next_value()`; `produce_next_value()` will only be called when the
    `watchable_value_t<bool>` is true, but it must never fail or block. */
    passive_producer_t(watchable_value_t<bool> *w) : available(w) { }
    virtual value_t produce_next_value() = 0;
    virtual ~passive_producer_t() { };

private:
    DISABLE_COPYING(passive_producer_t);
};

#endif /* __CONCURRENCY_QUEUE_PASSIVE_PRODUCER_HPP__ */
