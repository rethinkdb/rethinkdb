#include "clustering/dispatching_store.hpp"
#include "concurrency/pmap.hpp"

dispatching_store_t::dispatching_store_t() {
}

dispatching_store_t::~dispatching_store_t() {
    rassert(dispatchees.empty());
}

static void do_get_cas(int i, get_result_t *out, set_store_t *s, const store_key_t &key, const castime_t &castime) {
    get_result_t r = s->get_cas(key, castime);
    if (i == 0) *out = r;
}

get_result_t dispatching_store_t::get_cas(const store_key_t &key, castime_t castime) {
    rassert(!dispatchees.empty());
    get_result_t result;
    pimap(dispatchees.get_storage(key), dispatchees.end(), boost::bind(&do_get_cas, _2, &result, _1, key, castime));
    return result;
}

/* boost::bind() only works with up to 10-ish arguments. We have to combine them together to make it work. */
struct set_info_t {
    const store_key_t *key;
    data_provider_splitter_t *data_splitter;
    mcflags_t flags;
    exptime_t exptime;
    castime_t castime;
    add_policy_t add_policy;
    replace_policy_t replace_policy;
    cas_t old_cas;
};

static void do_sarc(int i, set_result_t *out, set_store_t *s, set_info_t *si) {
    set_result_t r = s->sarc(*si->key, si->data_splitter->branch(), si->flags, si->exptime, si->castime, si->add_policy, si->replace_policy, si->old_cas);
    if (i == 0) *out = r;
}

set_result_t dispatching_store_t::sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    rassert(!dispatchees.empty());
    data_provider_splitter_t splitter(data);
    set_info_t i;
    i.key = &key;
    i.data_splitter = &splitter;
    i.flags = flags;
    i.exptime = exptime;
    i.castime = castime;
    i.add_policy = add_policy;
    i.replace_policy = replace_policy;
    i.old_cas = old_cas;
    set_result_t result;
    pimap(dispatchees.get_storage(key), dispatchees.end(), boost::bind(&do_sarc, _2, &result, _1, &i));
    return result;
}

static void do_delete(int i, delete_result_t *out, set_store_t *s, const store_key_t &key, repli_timestamp timestamp) {
    delete_result_t r = s->delete_key(key, timestamp);
    if (i == 0) *out = r;
}

delete_result_t dispatching_store_t::delete_key(const store_key_t &key, repli_timestamp timestamp) {
    rassert(!dispatchees.empty());
    delete_result_t result;
    pimap(dispatchees.get_storage(key), dispatchees.end(), boost::bind(&do_delete, _2, &result, _1, key, timestamp));
    return result;
}

static void do_incr_decr(int i, incr_decr_result_t *out, set_store_t *s, incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    incr_decr_result_t r = s->incr_decr(kind, key, amount, castime);
    if (i == 0) *out = r;
}

incr_decr_result_t dispatching_store_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    rassert(!dispatchees.empty());
    incr_decr_result_t result;
    pimap(dispatchees.get_storage(key), dispatchees.end(), boost::bind(&do_incr_decr, _2, &result, _1, kind, key, amount, castime));
    return result;
}

static void do_append_prepend(int i, append_prepend_result_t *out, set_store_t *s, append_prepend_kind_t kind, const store_key_t &key, data_provider_splitter_t *data_splitter, castime_t castime) {
    append_prepend_result_t r = s->append_prepend(kind, key, data_splitter->branch(), castime);
    if (i == 0) *out = r;
}

append_prepend_result_t dispatching_store_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime) {
    rassert(!dispatchees.empty());
    data_provider_splitter_t splitter(data);
    append_prepend_result_t result;
    pimap(dispatchees.get_storage(key), dispatchees.end(), boost::bind(&do_append_prepend, _2, &result, _1, kind, key, &splitter, castime));
    return result;
}

dispatching_store_t::dispatchee_t::dispatchee_t(int bucket, dispatching_store_t *p, set_store_t *c)
    : client(c), parent(p), bucket(bucket) {
    parent->dispatchees.add_storage(bucket, c);
}

dispatching_store_t::dispatchee_t::~dispatchee_t() {
    parent->dispatchees.remove_storage(bucket);
}
