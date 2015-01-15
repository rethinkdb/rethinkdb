// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <functional>

#include "arch/runtime/runtime.hpp"

template <class value_t>
cross_thread_watchable_variable_t<value_t>::cross_thread_watchable_variable_t(const clone_ptr_t<watchable_t<value_t> > &w,
                                                                              threadnum_t _dest_thread) :
    original(w),
    watchable(this),
    watchable_thread(get_thread_id()),
    dest_thread(_dest_thread),
    rethreader(this),
    subs(std::bind(&cross_thread_watchable_variable_t<value_t>::on_value_changed, this)),
    deliver_cb(std::bind(&cross_thread_watchable_variable_t<value_t>::deliver, this, ph::_1)),
    messanger_pool(1, &value_producer, &deliver_cb) //Note it's very important that this coro_pool only have one worker it will be a race condition if it has more
{
    rassert(original->get_rwi_lock_assertion()->home_thread() == watchable_thread);
    typename watchable_t<value_t>::freeze_t freeze(original);
    value = original->get();
    subs.reset(original, &freeze);
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::on_value_changed() {
    value_producer.give_value(original->get());
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::deliver(value_t new_value) {
    on_thread_t thread_switcher(dest_thread);
    value = new_value;
    publisher_controller.publish(&cross_thread_watchable_variable_t<value_t>::call);
}

template <class value_t>
all_thread_watchable_variable_t<value_t>::all_thread_watchable_variable_t(
        const clone_ptr_t<watchable_t<value_t> > &input) {
    for (int i = 0; i < get_num_threads(); ++i) {
        vars.emplace_back(make_scoped<cross_thread_watchable_variable_t<value_t> >(
            input, threadnum_t(i)));
    }
}

template<class key_t, class value_t>
cross_thread_watchable_map_var_t<key_t, value_t>::cross_thread_watchable_map_var_t(
        watchable_map_t<key_t, value_t> *input,
        threadnum_t _output_thread) :
    output_var(input->get_all()),
    input_thread(get_thread_id()),
    output_thread(_output_thread),
    coro_running(false),
    rethreader(this),
    subs(input,
        [this](const key_t &key, const value_t *new_value) {
            this->on_change(key, new_value);
        }, false)
    { }

template<class key_t, class value_t>
void cross_thread_watchable_map_var_t<key_t, value_t>::flush() {
    mutex_assertion_t::acq_t acq(&lock);
    if (!coro_running) {
        /* All changes have been flushed */
        return;
    }
    cond_t waiter;
    queued_flushes.insert(&waiter);
    acq.reset();
    waiter.wait_lazily_unordered();
}

template<class key_t, class value_t>
void cross_thread_watchable_map_var_t<key_t, value_t>::on_change(
        const key_t &key, const value_t *value) {
    mutex_assertion_t::acq_t acq(&lock);
    if (value != nullptr) {
        queued_changes[key] = boost::optional<value_t>(*value);
    } else {
        queued_changes[key] = boost::optional<value_t>();
    }
    if (!coro_running) {
        coro_running = true;
        auto_drainer_t::lock_t keepalive(&drainer);
        coro_t::spawn_sometime([this, keepalive]() {
            this->ferry_changes(keepalive);
        });
    }
}

template<class key_t, class value_t>
void cross_thread_watchable_map_var_t<key_t, value_t>::ferry_changes(
        auto_drainer_t::lock_t keepalive) {
    guarantee(get_thread_id() == input_thread);
    while (true) {
        std::map<key_t, boost::optional<value_t> > changes;
        std::set<cond_t *> flushes;
        {
            mutex_assertion_t::acq_t acq(&lock);
            guarantee(coro_running);
            if ((queued_changes.empty() && queued_flushes.empty()) ||
                    keepalive.get_drain_signal()->is_pulsed()) {
                coro_running = false;
                return;
            } else {
                std::swap(changes, queued_changes);
                std::swap(flushes, queued_flushes);
            }
        }
        if (!changes.empty()) {
            on_thread_t thread_switcher(output_thread);
            for (const auto &pair : changes) {
                if (static_cast<bool>(pair.second)) {
                    output_var.set_key_no_equals(pair.first, *pair.second);
                } else {
                    output_var.delete_key(pair.first);
                }
            }
        }
        for (cond_t *c : flushes) {
            c->pulse();
        }
    }
}

template <class key_t, class value_t>
all_thread_watchable_map_var_t<key_t, value_t>::all_thread_watchable_map_var_t(
        watchable_map_t<key_t, value_t> *input) {
    for (int i = 0; i < get_num_threads(); ++i) {
        vars.emplace_back(make_scoped<cross_thread_watchable_map_var_t<key_t, value_t> >(
            input, threadnum_t(i)));
    }
}

template <class key_t, class value_t>
void all_thread_watchable_map_var_t<key_t, value_t>::flush() {
    for (int i = 0; i < get_num_threads(); ++i) {
        vars[i]->flush();
    }
}
