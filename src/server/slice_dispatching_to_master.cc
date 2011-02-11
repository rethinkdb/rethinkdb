#include "server/slice_dispatching_to_master.hpp"

#include "replication/master.hpp"

using replication::master_t;

/* get_store_t interface. */

get_result_t btree_slice_dispatching_to_master_t::get(store_key_t *key) {
    return slice_->get(key);
}

rget_result_ptr_t btree_slice_dispatching_to_master_t::rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open) {
    return slice_->rget(start, end, left_open, right_open);
}

/* set_store_t interface. */

get_result_t btree_slice_dispatching_to_master_t::get_cas(store_key_t *key, castime_t castime) {
    on_thread_t th(slice_->home_thread);
    if (master_) spawn_on_home(master_, boost::bind(&master_t::get_cas, _1, key, castime));
    return slice_->get_cas(key, castime);
}

set_result_t btree_slice_dispatching_to_master_t::sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    on_thread_t th(slice_->home_thread);

    if (master_) {
        buffer_borrowing_data_provider_t borrower(master_->home_thread, data);
        spawn_on_home(master_, boost::bind(&master_t::sarc, _1, key, borrower.side_provider(), flags, exptime, castime, add_policy, replace_policy, old_cas));
        return slice_->sarc(key, &borrower, flags, exptime, castime, add_policy, replace_policy, old_cas);
    } else {
        return slice_->sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
    }
}

incr_decr_result_t btree_slice_dispatching_to_master_t::incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime) {
    on_thread_t th(slice_->home_thread);
    if (master_) spawn_on_home(master_, boost::bind(&master_t::incr_decr, _1, kind, key, amount, castime));
    return slice_->incr_decr(kind, key, amount, castime);
}

append_prepend_result_t btree_slice_dispatching_to_master_t::append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime) {
    on_thread_t th(slice_->home_thread);
    if (master_) {
        buffer_borrowing_data_provider_t borrower(master_->home_thread, data);
        spawn_on_home(master_, boost::bind(&master_t::append_prepend, _1, kind, key, borrower.side_provider(), castime));
        return slice_->append_prepend(kind, key, &borrower, castime);
    } else {
        return slice_->append_prepend(kind, key, data, castime);
    }
}

delete_result_t btree_slice_dispatching_to_master_t::delete_key(store_key_t *key, repli_timestamp timestamp) {
    on_thread_t th(slice_->home_thread);
    if (master_) spawn_on_home(master_, boost::bind(&master_t::delete_key, _1, key, timestamp));
    return slice_->delete_key(key, timestamp);
}
