// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_
#define CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_

#include "arch/runtime/runtime.hpp"
#include "concurrency/watchable.hpp"
#include "concurrency/watchable_map.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/pump_coro.hpp"

/* `cross_thread_watchable_variable_t` is used to "proxy" a `watchable_t` from
one thread to another. Create the `cross_thread_watchable_variable_t` on the
source thread; then call `get_watchable()`, and you will get a watchable that is
usable on the `_dest_thread` that you passed to the constructor.

`cross_thread_watchable_map_var_t` is similar but for `watchable_map_t`.

See also: `cross_thread_signal_t`, which is the same thing for `signal_t`. */

template <class value_t>
class cross_thread_watchable_variable_t
{
public:
    cross_thread_watchable_variable_t(
        const clone_ptr_t<watchable_t<value_t> > &watchable,
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
    void deliver(signal_t *interruptor);

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
    be run after `deliver_pumper`'s destructor. */
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

    pump_coro_t deliver_pumper;

    /* The destructor for `subs` must be run before the destructor for `deliver_pumper`
    because `subs` calls `deliver_pumper.notify()` */
    typename watchable_t<value_t>::subscription_t subs;

    DISABLE_COPYING(cross_thread_watchable_variable_t);
};

/* `all_thread_watchable_variable_t` is like a `cross_thread_watchable_variable_t` except
that `get_watchable()` works on every thread, not just a specified thread. Internally it
constructs one `cross_thread_watchable_variable_t` for each thread, so it's a pretty
heavy-weight object. */

template<class value_t>
class all_thread_watchable_variable_t {
public:
    all_thread_watchable_variable_t(
        const clone_ptr_t<watchable_t<value_t> > &input);
    clone_ptr_t<watchable_t<value_t> > get_watchable() const {
        return vars[get_thread_id().threadnum]->get_watchable();
    }
private:
    std::vector<scoped_ptr_t<cross_thread_watchable_variable_t<value_t> > > vars;
};

template<class key_t, class value_t>
class cross_thread_watchable_map_var_t {
public:
    cross_thread_watchable_map_var_t(
        watchable_map_t<key_t, value_t> *input,
        threadnum_t output_thread);

    watchable_map_t<key_t, value_t> *get_watchable() {
        return &output_var;
    }

    /* Block until changes visible on this end at the moment `flush()` is called are
    visible on the other end. Caller is responsible for making sure that the
    `cross_thread_watchable_map_var_t` is not destroyed until after `flush()` returns. */
    void flush() {
        cond_t non_interruptor;
        deliver_pumper.flush(&non_interruptor);
    }

private:
    void on_change(const key_t &key, const value_t *value);
    void deliver(signal_t *interruptor);
    watchable_map_var_t<key_t, value_t> output_var;
    threadnum_t input_thread, output_thread;
    std::map<key_t, boost::optional<value_t> > queued_changes;

    /* This object's constructor rethreads our internal components to our other
    thread, and then reverses it in the destructor. It must be a separate object
    instead of logic in the constructor/destructor because its destructor must
    be run after `drainer`'s destructor. */
    class rethreader_t {
    public:
        explicit rethreader_t(cross_thread_watchable_map_var_t *p) : parent(p) {
            parent->output_var.rethread(parent->output_thread);
        }
        ~rethreader_t() {
            parent->output_var.rethread(parent->input_thread);
        }
    private:
        cross_thread_watchable_map_var_t *parent;
    } rethreader;

    pump_coro_t deliver_pumper;

    typename watchable_map_t<key_t, value_t>::all_subs_t subs;
};

/* `all_thread_watchable_map_var_t` is like a `cross_thread_watchable_map_var_t` except
that `get_watchable()` works on every thread, not just a specified thread. Internally it
constructs one `cross_thread_watchable_map_var_t` for each thread, so it's a pretty
heavy-weight object. */

template<class key_t, class value_t>
class all_thread_watchable_map_var_t {
public:
    all_thread_watchable_map_var_t(
        watchable_map_t<key_t, value_t> *input);
    watchable_map_t<key_t, value_t> *get_watchable() const {
        return vars[get_thread_id().threadnum]->get_watchable();
    }
    void flush();
private:
    std::vector<scoped_ptr_t<cross_thread_watchable_map_var_t<key_t, value_t> > > vars;
};

#include "concurrency/cross_thread_watchable.tcc"

#endif  // CONCURRENCY_CROSS_THREAD_WATCHABLE_HPP_
