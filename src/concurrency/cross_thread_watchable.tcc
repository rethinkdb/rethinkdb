
template <class value_t>
cross_thread_watchable_variable_t<value_t>::cross_thread_watchable_variable_t(const clone_ptr_t<watchable_t<value_t> > &w, int _dest_thread) : 
    original(w),
    watchable(this),
    watchable_thread(get_thread_id()),
    dest_thread(_dest_thread),
    subs(boost::bind(&cross_thread_watchable_variable_t<value_t>::on_value_changed, this, auto_drainer_t::lock_t(&drainer)))
{
    rwi_lock_assertion.rethread(dest_thread);
    publisher_controller.rethread(dest_thread);
    rassert(original->get_rwi_lock_assertion()->home_thread() == watchable_thread);
    typename watchable_t<value_t>::freeze_t freeze(original);
    value = original->get();
    subs.reset(original, &freeze);
}

template <class value_t>
cross_thread_watchable_variable_t<value_t>::~cross_thread_watchable_variable_t() {
    rwi_lock_assertion.rethread(watchable_thread);
    publisher_controller.rethread(watchable_thread);
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::on_value_changed(auto_drainer_t::lock_t keepalive) {
    coro_t::spawn_sometime(boost::bind(&cross_thread_watchable_variable_t<value_t>::deliver, this, original->get(), keepalive));
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::deliver(value_t new_value, UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t thread_switcher(dest_thread);
    rwi_lock_assertion_t::write_acq_t acquisition(&rwi_lock_assertion);
    value = new_value;
    publisher_controller.publish(&cross_thread_watchable_variable_t<value_t>::call);
}
