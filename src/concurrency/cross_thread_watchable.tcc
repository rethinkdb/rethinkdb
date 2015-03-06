// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <functional>

template <class value_t>
cross_thread_watchable_variable_t<value_t>::cross_thread_watchable_variable_t(
        const clone_ptr_t<watchable_t<value_t> > &w,
        threadnum_t _dest_thread) :
    original(w),
    watchable(this),
    watchable_thread(get_thread_id()),
    dest_thread(_dest_thread),
    rethreader(this),
    deliver_pumper([this](signal_t *interruptor) { this->deliver(interruptor); }),
    subs([this]() { this->deliver_pumper.notify(); })
{
    rassert(original->get_rwi_lock_assertion()->home_thread() == watchable_thread);
    typename watchable_t<value_t>::freeze_t freeze(original);
    value = original->get();
    subs.reset(original, &freeze);
}

template <class value_t>
void cross_thread_watchable_variable_t<value_t>::deliver(
        UNUSED signal_t *interruptor) {
    value_t temp = original->get();
    on_thread_t thread_switcher(dest_thread);
    value = temp;
    publisher_controller.publish([](const std::function<void()> &f) { f(); });
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
    rethreader(this),
    deliver_pumper([this](signal_t *interruptor) { this->deliver(interruptor); }),
    subs(input,
        [this](const key_t &key, const value_t *new_value) {
            this->on_change(key, new_value);
        }, false)
    { }

template<class key_t, class value_t>
void cross_thread_watchable_map_var_t<key_t, value_t>::on_change(
        const key_t &key, const value_t *value) {
    if (value != nullptr) {
        queued_changes[key] = boost::optional<value_t>(*value);
    } else {
        queued_changes[key] = boost::optional<value_t>();
    }
    deliver_pumper.notify();
}

template<class key_t, class value_t>
void cross_thread_watchable_map_var_t<key_t, value_t>::deliver(
        UNUSED signal_t *interruptor) {
    guarantee(get_thread_id() == input_thread);
    std::map<key_t, boost::optional<value_t> > local_changes;
    std::swap(local_changes, queued_changes);
    on_thread_t thread_switcher(output_thread);
    for (const auto &pair : local_changes) {
        if (static_cast<bool>(pair.second)) {
            output_var.set_key_no_equals(pair.first, *pair.second);
        } else {
            output_var.delete_key(pair.first);
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
