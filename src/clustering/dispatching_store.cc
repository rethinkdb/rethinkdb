#include "clustering/dispatching_store.hpp"
#include "concurrency/pmap.hpp"

/* The dispatching store takes sets over the wire and retrieves the data from
 * the correct storage nodes, it also takes gets from this machine and forwards
 * them directly to the correct storage node */

clustered_store_t::clustered_store_t() {
}

clustered_store_t::~clustered_store_t() {
    rassert(dispatchees.empty());
}

static void do_mutation(int i, mutation_result_t *out, std::pair<set_store_t *, get_store_t *> s, mutation_splitter_t *splitter, castime_t castime) {
    /* The reply that we send back to the client is the reply produced by the first dispatchee. */
    if (i == 0) {
        *out = s.first->change(splitter->branch(), castime);
    } else {
        mutation_result_t r = s.first->change(splitter->branch(), castime);
    }
}

mutation_result_t clustered_store_t::change(const mutation_t &mut, castime_t castime) {
    rassert(!dispatchees.empty());
    mutation_splitter_t splitter(mut);
    mutation_result_t res;
    pimap(dispatchees.get_storage(mut.get_key()), dispatchees.end(), boost::bind(&do_mutation, _2, &res, _1, &splitter, castime));
    return res;
}

get_result_t clustered_store_t::get(const store_key_t &key) {
    rassert(!dispatchees.empty());
    return (*dispatchees.get_storage(key)).second->get(key);
}

clustered_store_t::dispatchee_t::dispatchee_t(int bucket, clustered_store_t *p, std::pair<set_store_t *, get_store_t *> c)
    : client(c), parent(p), bucket(bucket) {
    parent->dispatchees.add_storage(bucket, c);
}

clustered_store_t::dispatchee_t::~dispatchee_t() {
    parent->dispatchees.remove_storage(bucket);
}
