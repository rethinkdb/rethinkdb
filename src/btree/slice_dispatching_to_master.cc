#include "btree/slice_dispatching_to_master.hpp"

#include "replication/master.hpp"

using replication::master_t;

/* store_t interface. */
store_t::get_result_t btree_slice_dispatching_to_master_t::get(store_key_t *key) {
    on_thread_t th(slice_->home_thread);
    return slice_->get(key);
}
store_t::get_result_t btree_slice_dispatching_to_master_t::get_cas(store_key_t *key, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&master_t::get_cas, _1, key, castime));
    return slice_->get_cas(key, castime);
}
store_t::rget_result_t btree_slice_dispatching_to_master_t::rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open) {
    on_thread_t th(slice_->home_thread);
    return slice_->rget(start, end, left_open, right_open);
}
store_t::set_result_t btree_slice_dispatching_to_master_t::sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t maybe_castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);

    if (masterstore_) {
        buffer_borrowing_data_provider_t borrower(masterstore_->home_thread, data);
        spawn_on_home(masterstore_, boost::bind(&master_t::sarc, _1, key, borrower.side_provider(), flags, exptime, castime, add_policy, replace_policy, old_cas));
        return slice_->sarc(key, &borrower, flags, exptime, castime, add_policy, replace_policy, old_cas);
    } else {
        return slice_->sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
    }
}

store_t::incr_decr_result_t btree_slice_dispatching_to_master_t::incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&master_t::incr_decr, _1, kind, key, amount, castime));
    return slice_->incr_decr(kind, key, amount, castime);
}

store_t::append_prepend_result_t btree_slice_dispatching_to_master_t::append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t maybe_castime) {
    on_thread_t th(slice_->home_thread);
    castime_t castime = generate_if_necessary(maybe_castime);
    if (masterstore_) {
        buffer_borrowing_data_provider_t borrower(masterstore_->home_thread, data);
        spawn_on_home(masterstore_, boost::bind(&master_t::append_prepend, _1, kind, key, borrower.side_provider(), castime));
        return slice_->append_prepend(kind, key, &borrower, castime);
    } else {
        return slice_->append_prepend(kind, key, data, castime);
    }
}

store_t::delete_result_t btree_slice_dispatching_to_master_t::delete_key(store_key_t *key, repli_timestamp maybe_timestamp) {
    on_thread_t th(slice_->home_thread);
    repli_timestamp timestamp = generate_if_necessary(maybe_timestamp);
    if (masterstore_) spawn_on_home(masterstore_, boost::bind(&master_t::delete_key, _1, key, timestamp));
    return slice_->delete_key(key, timestamp);
}


castime_t btree_slice_dispatching_to_master_t::gen_castime() {
    repli_timestamp timestamp = current_time();
    cas_t cas = (uint64_t(timestamp.time) << 32) | ++cas_counter_;
    return castime_t(cas, timestamp);
}

castime_t btree_slice_dispatching_to_master_t::generate_if_necessary(castime_t castime) {
    return castime.is_dummy() ? gen_castime() : castime;
}

repli_timestamp btree_slice_dispatching_to_master_t::generate_if_necessary(repli_timestamp timestamp) {
    return timestamp.time == repli_timestamp::invalid.time ? current_time() : timestamp;
}
