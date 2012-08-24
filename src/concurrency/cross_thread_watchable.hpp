#ifndef CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_
#define CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_

#include "concurrency/watchable.hpp"
#include "concurrency/auto_drainer.hpp"
#include "timestamps.hpp"

template <class value_t>
class cross_thread_watchable_variable_t
{
public:
    cross_thread_watchable_variable_t(const clone_ptr_t<watchable_t<value_t> > &watchable, int _dest_thread);

    clone_ptr_t<watchable_t<value_t> > get_watchable() {
        return clone_ptr_t<watchable_t<value_t> >(watchable.clone());
    }

    int home_thread() { return watchable_thread; }

private:
    friend class cross_thread_watcher_subscription_t;
    void on_value_changed(auto_drainer_t::lock_t keepalive);
    void deliver(value_t new_value, transition_timestamp_t ts, auto_drainer_t::lock_t keepalive);

    static void call(const boost::function<void()> &f) {
        f();
    }

    class w_t : public watchable_t<value_t> {
    public:
        explicit w_t(cross_thread_watchable_variable_t<value_t> *p) : parent(p) { }

        w_t *clone() {
            return new w_t(parent);
        }
        value_t get() {
            return parent->value;
        }
        publisher_t<boost::function<void()> > *get_publisher() {
            return parent->publisher_controller.get_publisher();
        }
        rwi_lock_assertion_t *get_rwi_lock_assertion() {
            return &parent->rwi_lock_assertion;
        }
        cross_thread_watchable_variable_t<value_t> *parent;
    };

    clone_ptr_t<watchable_t<value_t> > original;
    publisher_controller_t<boost::function<void()> > publisher_controller;
    rwi_lock_assertion_t rwi_lock_assertion;
    value_t value;
    w_t watchable;

    state_timestamp_t watchable_thread_timestamp, dest_thread_timestamp;

    int watchable_thread, dest_thread;

    /* This object's constructor rethreads our internal components to our other
    thread, and then reverses it in the destructor. It must be a separate object
    instead of logic in the constructor/destructor because its destructor must
    be run after `drainer`'s destructor. */
    class rethreader_t {
    public:
        explicit rethreader_t(cross_thread_watchable_variable_t *p) :
            parent(p)
        {
            parent->rwi_lock_assertion.rethread(parent->dest_thread);
            parent->publisher_controller.rethread(parent->dest_thread);
        }
        ~rethreader_t() {
            parent->rwi_lock_assertion.rethread(parent->watchable_thread);
            parent->publisher_controller.rethread(parent->watchable_thread);
        }
    private:
        cross_thread_watchable_variable_t *parent;
    } rethreader;

    /* `drainer` makes sure we don't shut down with an update still in flight */
    auto_drainer_t drainer;

    /* The destructor for `subs` must be run before the destructor for `drainer`
    because `drainer`'s destructor will block until all the
    `auto_drainer_t::lock_t` objects are gone, and `subs`'s callback holds an
    `auto_drainer_t::lock_t`. */
    typename watchable_t<value_t>::subscription_t subs;
};

#include "concurrency/cross_thread_watchable.tcc"

#endif  // CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_
