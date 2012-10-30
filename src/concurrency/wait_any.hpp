// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_WAIT_ANY_HPP_
#define CONCURRENCY_WAIT_ANY_HPP_

#include <vector>

#include "concurrency/signal.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/object_buffer.hpp"


/* Monitors multiple signals; becomes pulsed if any individual signal becomes
pulsed. */

class wait_any_subscription_t;

class wait_any_t : public signal_t {
public:
    wait_any_t();
    explicit wait_any_t(const signal_t *s1);
    wait_any_t(const signal_t *s1, const signal_t *s2);
    wait_any_t(const signal_t *s1, const signal_t *s2, const signal_t *s3);
    wait_any_t(const signal_t *s1, const signal_t *s2, const signal_t *s3, const signal_t *s4);
    wait_any_t(const signal_t *s1, const signal_t *s2, const signal_t *s3, const signal_t *s4, const signal_t *s5);

    ~wait_any_t();

    void add(const signal_t *s);
private:
    class wait_any_subscription_t : public signal_t::subscription_t, public intrusive_list_node_t<wait_any_subscription_t> {
    public:
        wait_any_subscription_t(wait_any_t *_parent, bool _alloc_on_heap) : parent(_parent), alloc_on_heap(_alloc_on_heap) { }
        virtual void run();
        bool on_heap() const { return alloc_on_heap; }
    private:
        wait_any_t *parent;
        bool alloc_on_heap;
        DISABLE_COPYING(wait_any_subscription_t);
    };

    static const size_t default_preallocated_subs = 3;

    void pulse_if_not_already_pulsed();

    intrusive_list_t<wait_any_subscription_t> subs;

    object_buffer_t<wait_any_subscription_t> sub_storage[default_preallocated_subs];

    DISABLE_COPYING(wait_any_t);
};

/* Waits for the first signal to become pulsed. If the second signal becomes
pulsed, stops waiting and throws `interrupted_exc_t`. */
void wait_interruptible(const signal_t *signal, const signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

#endif /* CONCURRENCY_WAIT_ANY_HPP_ */
