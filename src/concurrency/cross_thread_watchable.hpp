// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_
#define CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_

#include "concurrency/watchable.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/single_value_producer.hpp"
#include "concurrency/coro_pool.hpp"

/* `cross_thread_watchable_variable_t` is used to "proxy" a `watchable_t` from
one thread to another. Create the `cross_thread_watchable_variable_t` on the
source thread; then call `get_watchable()`, and you will get a watchable that is
usable on the `_dest_thread` that you passed to the constructor.

See also: `cross_thread_signal_t`, which is the same thing for `signal_t`. */

template <class value_t>
class cross_thread_watchable_variable_t
{
public:
    cross_thread_watchable_variable_t(const clone_ptr_t<watchable_t<value_t> > &watchable,
                                      threadnum_t _dest_thread);

    clone_ptr_t<watchable_t<value_t> > get_watchable() {
        return clone_ptr_t<watchable_t<value_t> >(watchable.clone());
    }

    threadnum_t home_thread() { return watchable_thread; }

    template <class Callable>
    void apply_read(Callable &&read) {
        ASSERT_NO_CORO_WAITING;
        const value_t *const_value = &value;
        read(const_value);
    }

private:
    friend class cross_thread_watcher_subscription_t;
    void on_value_changed();
    void deliver(value_t new_value);

    static void call(const std::function<void()> &f) {
        f();
    }

    class w_t : public watchable_t<value_t> {
    public:
        explicit w_t(cross_thread_watchable_variable_t<value_t> *p) : parent(p) { }

        w_t *clone() const {
            return new w_t(parent);
        }
        value_t get() {
            return parent->value;
        }
        void apply_read(const std::function<void(const value_t*)> &read) {
            return parent->apply_read(read);
        }
        publisher_t<std::function<void()> > *get_publisher() {
            return parent->publisher_controller.get_publisher();
        }
        rwi_lock_assertion_t *get_rwi_lock_assertion() {
            return &parent->rwi_lock_assertion;
        }
        void rethread(threadnum_t thread) {
            home_thread_mixin_t::real_home_thread = thread;
        }
        cross_thread_watchable_variable_t<value_t> *parent;
    };

    clone_ptr_t<watchable_t<value_t> > original;
    publisher_controller_t<std::function<void()> > publisher_controller;
    rwi_lock_assertion_t rwi_lock_assertion;
    value_t value;
    w_t watchable;

    threadnum_t watchable_thread;
    threadnum_t dest_thread;

    /* This object's constructor rethreads our internal components to our other
    thread, and then reverses it in the destructor. It must be a separate object
    instead of logic in the constructor/destructor because its destructor must
    be run after `drainer`'s destructor. */
    class rethreader_t {
    public:
        explicit rethreader_t(cross_thread_watchable_variable_t *p) :
            parent(p)
        {
            parent->watchable.rethread(parent->dest_thread);
            parent->rwi_lock_assertion.rethread(parent->dest_thread);
            parent->publisher_controller.rethread(parent->dest_thread);
        }
        ~rethreader_t() {
            parent->watchable.rethread(parent->watchable_thread);
            parent->rwi_lock_assertion.rethread(parent->watchable_thread);
            parent->publisher_controller.rethread(parent->watchable_thread);
        }
    private:
        cross_thread_watchable_variable_t *parent;
    } rethreader;

    /* The destructor for `subs` must be run before the destructor for `drainer`
    because `drainer`'s destructor will block until all the
    `auto_drainer_t::lock_t` objects are gone, and `subs`'s callback holds an
    `auto_drainer_t::lock_t`. */
    typename watchable_t<value_t>::subscription_t subs;

    single_value_producer_t<value_t> value_producer;
    std_function_callback_t<value_t> deliver_cb;
    coro_pool_t<value_t> messanger_pool;

    DISABLE_COPYING(cross_thread_watchable_variable_t);
};

#include "concurrency/cross_thread_watchable.tcc"

#endif  // CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_
