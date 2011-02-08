#include "btree/slice_dispatching_to_masterstore.hpp"

#include "replication/masterstore.hpp"

using replication::masterstore_t;

/* store_t interface. */
store_t::get_result_t btree_slice_dispatching_to_masterstore_t::get(store_key_t *key) {
    on_thread_t th(slice_->home_thread);
    return slice_->get(key);
}
store_t::get_result_t btree_slice_dispatching_to_masterstore_t::get_cas(store_key_t *key, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::get_cas, _1, key, castime));
    return slice_->get_cas(key, castime);
}
store_t::rget_result_t btree_slice_dispatching_to_masterstore_t::rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open) {
    on_thread_t th(slice_->home_thread);
    return slice_->rget(start, end, left_open, right_open);
}
store_t::set_result_t btree_slice_dispatching_to_masterstore_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);

    if (masterstore_) {
        buffer_borrowing_data_provider_t borrower(masterstore_->home_thread, data);
        spawn_on_home(masterstore_, boost::bind(&masterstore_t::set, _1, key, borrower.side_provider(), flags, exptime, castime));
        return slice_->set(key, &borrower, flags, exptime, castime);
    } else {
        return slice_->set(key, data, flags, exptime, castime);
    }
}
store_t::set_result_t btree_slice_dispatching_to_masterstore_t::add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::add, _1, key, data, flags, exptime, castime));
    return slice_->add(key, data, flags, exptime, castime);
}
store_t::set_result_t btree_slice_dispatching_to_masterstore_t::replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::replace, _1, key, data, flags, exptime, castime));
    return slice_->replace(key, data, flags, exptime, castime);
}
store_t::set_result_t btree_slice_dispatching_to_masterstore_t::cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::cas, _1, key, data, flags, exptime, unique, castime));
    return slice_->cas(key, data, flags, exptime, unique, castime);
}
store_t::incr_decr_result_t btree_slice_dispatching_to_masterstore_t::incr(store_key_t *key, unsigned long long amount, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::incr, _1, key, amount, castime));
    return slice_->incr(key, amount, castime);
}
store_t::incr_decr_result_t btree_slice_dispatching_to_masterstore_t::decr(store_key_t *key, unsigned long long amount, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::decr, _1, key, amount, castime));
    return slice_->decr(key, amount, castime);
}
store_t::append_prepend_result_t btree_slice_dispatching_to_masterstore_t::append(store_key_t *key, data_provider_t *data, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::append, _1, key, data, castime));
    return slice_->append(key, data, castime);
}
store_t::append_prepend_result_t btree_slice_dispatching_to_masterstore_t::prepend(store_key_t *key, data_provider_t *data, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::prepend, _1, key, data, castime));
    return slice_->prepend(key, data, castime);
}
store_t::delete_result_t btree_slice_dispatching_to_masterstore_t::delete_key(store_key_t *key, repli_timestamp maybe_timestamp) {
    on_thread_t th(slice_->home_thread);
    repli_timestamp timestamp = generate_if_necessary(maybe_timestamp);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&masterstore_t::delete_key, _1, key, timestamp));
    return slice_->delete_key(key, timestamp);
}


castime_t btree_slice_dispatching_to_masterstore_t::gen_castime() {
    repli_timestamp timestamp = current_time();
    cas_t cas = (uint64_t(timestamp.time) << 32) | ++cas_counter_;
    return castime_t(cas, timestamp);
}

castime_t btree_slice_dispatching_to_masterstore_t::generate_if_necessary(castime_t castime) {
    return castime.is_dummy() ? gen_castime() : castime;
}

repli_timestamp btree_slice_dispatching_to_masterstore_t::generate_if_necessary(repli_timestamp timestamp) {
    return timestamp.time == repli_timestamp::invalid.time ? current_time() : timestamp;
}
