#include "arch/runtime/runtime.hpp"

template <class value_t>
cross_thread_watchable_variable_t<value_t>::cross_thread_watchable_variable_t(const clone_ptr_t<watchable_t<value_t> > &w, int _dest_thread) : 
    original(w),
    watchable(this),
    watchable_thread_timestamp(state_timestamp_t::zero()),
    dest_thread_timestamp(state_timestamp_t::zero()),
    watchable_thread(get_thread_id()),
    dest_thread(_dest_thread),
    rethreader(this),
    subs(boost::bind(&cross_thread_watchable_variable_t<value_t>::on_value_changed, this, auto_drainer_t::lock_t(&drainer)))
{
    rassert(original->get_rwi_lock_assertion()->home_thread() == watchable_thread);
    typename watchable_t<value_t>::freeze_t freeze(original);
    value = original->get();
    subs.reset(original, &freeze);
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::on_value_changed(auto_drainer_t::lock_t keepalive) {
    transition_timestamp_t ts = transition_timestamp_t::starting_from(watchable_thread_timestamp);
    watchable_thread_timestamp = ts.timestamp_after();
    coro_t::spawn_sometime(boost::bind(&cross_thread_watchable_variable_t<value_t>::deliver, this, original->get(), ts, keepalive));
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::deliver(value_t new_value, transition_timestamp_t ts, UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t thread_switcher(dest_thread);
    rwi_lock_assertion_t::write_acq_t acquisition(&rwi_lock_assertion);
    if (dest_thread_timestamp <= ts.timestamp_before()) {
        dest_thread_timestamp = ts.timestamp_after();
        value = new_value;
        publisher_controller.publish(&cross_thread_watchable_variable_t<value_t>::call);
    }
}
