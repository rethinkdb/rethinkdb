#ifndef __CONCURRENCY_WATCHABLE_HPP__
#define __CONCURRENCY_WATCHABLE_HPP__

#include "concurrency/pubsub.hpp"

/* Forward declaration */

template<class value_t>
class watchable_controller_t;

/* A `watchable_t<T>` is a value of type `T` plus notifications when that value
changes.

`get()` return the current value. Construct a `watchable_t<T>::subscription_t`
to receive notifications when the value changes. Notifications will be delivered
by calling the function you pass to the `watchable_t<T>::subscription_t`
constructor. The function must not block. If you attempt to change the value or
add or remove subscriptions in the callback, your attempt will block. (That's
not a contradiction because you can spawn a coroutine from within the callback.)
*/

template<class value_t>
class watchable_t :
    public home_thread_mixin_t
{
public:
    value_t get() const;

    class subscription_t :
        public home_thread_mixin_t
    {
    public:
        subscription_t(boost::function<void()> cb);
        subscription_t(boost::function<void()> cb, watchable_t *w);
        void resubscribe(watchable_t *w);
        void unsubscribe();

    private:
        publisher_t<boost::function<void()> >::subscription_t subs;
        DISABLE_COPYING(subscription_t);
    };

private:
    /* `watchable_t` is pretty much an empty shell that refers back to the
    `watchable_controller_t` it's associated with. */

    friend class subscription_t;
    friend class watchable_controller_t<value_t>;

    watchable_controller_t<value_t> *parent;

    DISABLE_COPYING(watchable_t);
};

template<class value_t>
class watchable_controller_t :
    public home_thread_mixin_t
{
public:
    watchable_controller_t(value_t v, mutex_t *m) :
        watchable(this),
        mutex(m),
        value(v),
        publisher_controller(m) { }

    watchable_t<value_t> *get_watchable() {
        return &watchable;
    }

    void change(const value_t &new_value, mutex_acquisition_t *proof) {
        proof->assert_is_holding(mutex);
        value = new_value;
        publisher_controller.publish(&watchable_controller_t::call);
    }

    void rethread(int new_thread) {
        real_home_thread = new_thread;
        watchable.real_home_thread = new_thread;
        publisher_controller.rethread(new_thread);
    }

private:
    static void call(boost::function<void()> &fun) {
        fun();
    }

    watchable_t<value_t> watchable;

    mutex_t *mutex;

    /* The current value */
    value_t value;

    /* The publisher we use to send out change notifications */
    publisher_controller_t<boost::function<void()> > publisher_controller;

    DISABLE_COPYING(watchable_controller_t);
};

/* These are defined down here because they need to be able to access the guts
of `watchable_controller_t`. */

template<class value_t>
value_t watchable_t<value_t>::get() const {
    assert_thread();
    return parent->value;
}

template<class value_t>
watchable_t<value_t>::subscription_t::subscription_t(boost::function<void()> cb) :
    subs(cb) { }

template<class value_t>
watchable_t<value_t>::subscription_t::subscription_t(boost::function<void()> cb, watchable_t *w) :
    subs(cb, w->publisher_controller.get_publisher()) { }

template<class value_t>
void watchable_t<value_t>::subscription_t::resubscribe(watchable_t *w) {
    subs.resubscribe(w->publisher_controller.get_publisher());
}

template<class value_t>
void watchable_t<value_t>::subscription_t::unsubscribe() {
    subs.unsubscribe();
}

#endif /* __CONCURRENCY_WATCHABLE_HPP__ */
