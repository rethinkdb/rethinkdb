#include "store.hpp"

struct mutation_get_key_functor_t : public boost::static_visitor<store_key_t> {
    store_key_t operator()(const get_cas_mutation_t &m) { return m.key; }
    store_key_t operator()(const set_mutation_t &m) { return m.key; }
    store_key_t operator()(const delete_mutation_t &m) { return m.key; }
    store_key_t operator()(const incr_decr_mutation_t &m) { return m.key; }
    store_key_t operator()(const append_prepend_mutation_t &m) { return m.key; }
};

store_key_t mutation_t::get_key() const {
    mutation_get_key_functor_t functor;
    return boost::apply_visitor(functor, mutation);
}

get_result_t set_store_interface_t::get_cas(const store_key_t &key) {
    get_cas_mutation_t mut;
    mut.key = key;
    return boost::get<get_result_t>(change(mut).result);
}

set_result_t set_store_interface_t::sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    set_mutation_t mut;
    mut.key = key;
    mut.data = data;
    mut.flags = flags;
    mut.exptime = exptime;
    mut.add_policy = add_policy;
    mut.replace_policy = replace_policy;
    mut.old_cas = old_cas;
    return boost::get<set_result_t>(change(mut).result);
}

incr_decr_result_t set_store_interface_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount) {
    incr_decr_mutation_t mut;
    mut.kind = kind;
    mut.key = key;
    mut.amount = amount;
    return boost::get<incr_decr_result_t>(change(mut).result);
}

append_prepend_result_t set_store_interface_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data) {
    append_prepend_mutation_t mut;
    mut.kind = kind;
    mut.key = key;
    mut.data = data;
    return boost::get<append_prepend_result_t>(change(mut).result);
}

delete_result_t set_store_interface_t::delete_key(const store_key_t &key) {
    delete_mutation_t mut;
    mut.key = key;
    return boost::get<delete_result_t>(change(mut).result);
}

timestamping_set_store_interface_t::timestamping_set_store_interface_t(set_store_t *target)
    : target(target), cas_counter(0) { }

mutation_result_t timestamping_set_store_interface_t::change(const mutation_t &mutation) {
    on_thread_t thread_switcher(home_thread);
    return target->change(mutation, make_castime());
}

castime_t timestamping_set_store_interface_t::make_castime() {
    repli_timestamp timestamp = current_time();

    /* The cas-value includes the current time and a counter. The time is so that we don't assign
    the same CAS twice across multiple runs of the database. The counter is so that we don't assign
    the same CAS twice to two requests received in the same second. */
    cas_t cas = (uint64_t(timestamp.time) << 32) ^ (++cas_counter);

    return castime_t(cas, timestamp);
}
