#include "store.hpp"

timestamping_set_store_interface_t::timestamping_set_store_interface_t(set_store_t *target)
    : target(target), cas_counter(0) { }

get_result_t timestamping_set_store_interface_t::get_cas(store_key_t *key) {
    on_thread_t thread_switcher(home_thread);
    return target->get_cas(key, make_castime());
}

set_result_t timestamping_set_store_interface_t::sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    on_thread_t thread_switcher(home_thread);
    return target->sarc(key, data, flags, exptime, make_castime(), add_policy, replace_policy, old_cas);
}

incr_decr_result_t timestamping_set_store_interface_t::incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount) {
    on_thread_t thread_switcher(home_thread);
    return target->incr_decr(kind, key, amount, make_castime());
}

append_prepend_result_t timestamping_set_store_interface_t::append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data) {
    on_thread_t thread_switcher(home_thread);
    return target->append_prepend(kind, key, data, make_castime());
}

delete_result_t timestamping_set_store_interface_t::delete_key(store_key_t *key) {
    on_thread_t thread_switcher(home_thread);
    return target->delete_key(key, current_time());
}

castime_t timestamping_set_store_interface_t::make_castime() {
    repli_timestamp timestamp = current_time();

    /* The cas-value includes the current time and a counter. The time is so that we don't assign
    the same CAS twice across multiple runs of the database. The counter is so that we don't assign
    the same CAS twice to two requests received in the same second. */
    cas_t cas = (uint64_t(timestamp.time) << 32) | (++cas_counter);

    return castime_t(cas, timestamp);
}
