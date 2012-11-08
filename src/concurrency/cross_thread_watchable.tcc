// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/runtime.hpp"

template <class value_t>
cross_thread_watchable_variable_t<value_t>::cross_thread_watchable_variable_t(const clone_ptr_t<watchable_t<value_t> > &w, int _dest_thread) : 
    original(w),
    watchable(this),
    watchable_thread(get_thread_id()),
    dest_thread(_dest_thread),
    rethreader(this),
    subs(boost::bind(&cross_thread_watchable_variable_t<value_t>::on_value_changed, this)),
    deliver_cb(boost::bind(&cross_thread_watchable_variable_t<value_t>::deliver, this, _1)),
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
