// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_WATCHABLE_HPP_
#define CONCURRENCY_WATCHABLE_HPP_

#include <functional>

#include "concurrency/interruptor.hpp"
#include "concurrency/mutex_assertion.hpp"
#include "concurrency/pubsub.hpp"
#include "concurrency/signal.hpp"
#include "containers/clone_ptr.hpp"
#include "utils.hpp"

/* `watchable_t` represents a variable that you can get the value of and also
subscribe to further changes to the value. To get the value of a `watchable_t`,
just call `get()`. To subscribe to further changes to the value, construct a
`watchable_t::freeze_t` and then construct a `watchable_t::subscription_t`,
passing it the `watchable_t` and the `freeze_t`.

To create a `watchable_t`, construct a `watchable_variable_t` and then call its
`get_watchable()` method. You can change the value of the `watchable_t` by
calling `watchable_variable_t::set_value()`.

`watchable_t` has a home thread; it can only be accessed from that thread. If
you need to access it from another thread, consider using
`cross_thread_watchable_variable_t` to create a proxy-watchable with a different
home thread. */

template <class value_t> class watchable_t;
template <class value_t> class watchable_freeze_t;

template <class value_t>
class watchable_subscription_t {
public:
    /* The `std::function<void()>` passed to the constructor is a callback
       that will be called whenever the value changes. The callback must not
       block, and it must not create or destroy `subscription_t` objects. */

    explicit watchable_subscription_t(const std::function<void()> &f)
        : subscription(f)
    { }

    watchable_subscription_t(const std::function<void()> &f, const clone_ptr_t<watchable_t<value_t> > &watchable,
                             watchable_freeze_t<value_t> *freeze)
        : subscription(f, watchable->get_publisher()) {
        watchable->assert_thread();
        freeze->rwi_lock_acquisition.assert_is_holding(watchable->get_rwi_lock_assertion());
    }

    void reset() {
        subscription.reset();
    }

    void reset(const clone_ptr_t<watchable_t<value_t> > &watchable, watchable_freeze_t<value_t> *freeze) {
        watchable->assert_thread();
        freeze->rwi_lock_acquisition.assert_is_holding(watchable->get_rwi_lock_assertion());
        subscription.reset(watchable->get_publisher());
    }

private:
    typename publisher_t<std::function<void()> >::subscription_t subscription;

    DISABLE_COPYING(watchable_subscription_t);
};


/* The purpose of the `freeze_t` is to assert that the value of the
   `watchable_t` doesn't change for as long as it exists. You should not block
   while the `freeze_t` exists. It's kind of like `mutex_assertion_t`.

   A common pattern is to construct a `freeze_t`, then call `get()` to
   initialize some object or objects that mirrors the `watchable_t`'s value,
   then construct a `subscription_t` to continually keep in sync with the
   `watchable_t`, then destroy the `freeze_t`. If it weren't for the
   `freeze_t`, a change might "slip through the cracks" if it happened after
   `get()` but before constructing the `subscription_t`. If you constructed the
   `subscription_t` before calling `get()`, then the callback might get run
   before the objects were first initialized. */

template <class value_t>
class watchable_freeze_t {
public:
    explicit watchable_freeze_t(const clone_ptr_t<watchable_t<value_t> > &watchable)
        : rwi_lock_acquisition(watchable->get_rwi_lock_assertion())
    {
        watchable->assert_thread();
    }

    void assert_is_holding(const clone_ptr_t<watchable_t<value_t> > &watchable) {
        rwi_lock_acquisition.assert_is_holding(watchable->get_rwi_lock_assertion());
    }

private:
    friend class watchable_subscription_t<value_t>;
    rwi_lock_assertion_t::read_acq_t rwi_lock_acquisition;

    DISABLE_COPYING(watchable_freeze_t);
};

template <class value_t>
class watchable_t : public home_thread_mixin_t {
public:
    typedef watchable_freeze_t<value_t> freeze_t;
    typedef watchable_subscription_t<value_t> subscription_t;

    virtual ~watchable_t() { }
    virtual watchable_t *clone() const = 0;

    virtual value_t get() = 0;

    /* Applies `read` to the current value of the watchable. `read` must not block
    and cannot change the value. */
    virtual void apply_read(const std::function<void(const value_t*)> &read) = 0;

    /* These are internal; the reason they're public is so that `subview()` and
    similar things can be implemented. */
    virtual publisher_t<std::function<void()> > *get_publisher() = 0;
    virtual rwi_lock_assertion_t *get_rwi_lock_assertion() = 0;

    /* `subview()` returns another `watchable_t` whose value is derived from
    this one by calling `lens` on it. */
    template<class callable_type>
    clone_ptr_t<watchable_t<typename std::result_of<callable_type(value_t)>::type> > subview(const callable_type &lens);
    /* `incremental_subview()` is like `subview()`. However it takes an incremental lens.
    An incremental lens has the signature
    `bool(const input_type &, result_type *current_out)`.
    In contrast to a regular (non-incremental) lens it therefore has access to
    the current value of the output value and can modify it in-place.
    It should return true if `current_out` has been modified, and false otherwise.
    We have two variants of this. The first one is a short-cut for incremental lenses
    which contain a `result_type` typedef. In this case, result_type does not have
    to be specified explicitly when calling `incremental_subview()`. Otherwise
    the second variant must be used and `result_type` be specified as a template
    parameter. */
    template<class callable_type>
    clone_ptr_t<watchable_t<typename callable_type::result_type> > incremental_subview(const callable_type &lens);
    template<class result_type, class callable_type>
    clone_ptr_t<watchable_t<result_type> > incremental_subview(const callable_type &lens);

    /* `run_until_satisfied()` repeatedly calls `fun` on the current value of
    `this` until either `fun` returns `true` or `interruptor` is pulsed. It's
    efficient because it only retries `fun` when the value changes. */
    template<class callable_type>
    void run_until_satisfied(const callable_type &fun, signal_t *interruptor,
            int64_t nap_before_retry_ms = 0) THROWS_ONLY(interrupted_exc_t);

protected:
    watchable_t() { }

private:
    DISABLE_COPYING(watchable_t);
};

/* `run_until_satisfied_2()` repeatedly calls `fun(a->get(), b->get())` until
`fun` returns `true` or `interruptor` is pulsed. It's efficient because it only
retries `fun` when the value of `a` or `b` changes. */
template<class a_type, class b_type, class callable_type>
void run_until_satisfied_2(
        const clone_ptr_t<watchable_t<a_type> > &a,
        const clone_ptr_t<watchable_t<b_type> > &b,
        const callable_type &fun,
        signal_t *interruptor,
        int64_t nap_before_retry_ms = 0) THROWS_ONLY(interrupted_exc_t);

inline void call_function(const std::function<void()> &f) {
    f();
}

template <class value_t>
class watchable_variable_t {
public:
    explicit watchable_variable_t(const value_t &_value)
        : value(_value), watchable(this)
    { }

    clone_ptr_t<watchable_t<value_t> > get_watchable() {
        return clone_ptr_t<watchable_t<value_t> >(watchable.clone());
    }

    void set_value(const value_t &_value) {
        DEBUG_VAR rwi_lock_assertion_t::write_acq_t acquisition(&rwi_lock_assertion);
        /* Sometimes we have `==` but not `!=` for whatever reason */
        if (!(value == _value)) {
            value = _value;
            publisher_controller.publish(&call_function);
        }
    }

    void set_value_no_equals(const value_t &_value) {
        DEBUG_VAR rwi_lock_assertion_t::write_acq_t acquisition(&rwi_lock_assertion);
        value = _value;
        publisher_controller.publish(&call_function);
    }

    // Applies an atomic modification to the value.
    // `op` must return true if the value was modified,
    // and should return false otherwise.
    void apply_atomic_op(const std::function<bool(value_t*)> &op) {  // NOLINT(readability/casting)
        DEBUG_VAR rwi_lock_assertion_t::write_acq_t acquisition(&rwi_lock_assertion);
        bool was_modified;
        {
            ASSERT_NO_CORO_WAITING;
            was_modified = op(&value);
        }
        if (was_modified) {
            publisher_controller.publish(&call_function);
        }
    }

    // This is similar to apply_atomic_op, except that the operation
    // cannot modify the value.
    void apply_read(const std::function<void(const value_t*)> &read) {
        ASSERT_NO_CORO_WAITING;
        read(&value);
    }

    /* This returns a const reference to the current value. The caller is responsible for
    ensuring that nothing calls `set_value()` or `apply_atomic_op()` while this reference
    exists. */
    const value_t &get_ref() {
        return value;
    }

    value_t get() {
        return value;
    }

private:
    class w_t : public watchable_t<value_t> {
    public:
        explicit w_t(watchable_variable_t<value_t> *p) : parent(p) { }
        w_t *clone() const {
            return new w_t(parent);
        }
        value_t get() {
            return parent->value;
        }
        publisher_t<std::function<void()> > *get_publisher() {
            return parent->publisher_controller.get_publisher();
        }
        rwi_lock_assertion_t *get_rwi_lock_assertion() {
            return &parent->rwi_lock_assertion;
        }
        void apply_read(const std::function<void(const value_t*)> &read) {
            parent->apply_read(read);
        }
        watchable_variable_t<value_t> *parent;
    };

    publisher_controller_t<std::function<void()> > publisher_controller;
    value_t value;
    rwi_lock_assertion_t rwi_lock_assertion;

    w_t watchable;

    DISABLE_COPYING(watchable_variable_t);
};

#include "concurrency/watchable.tcc"

#endif /* CONCURRENCY_WATCHABLE_HPP_ */
