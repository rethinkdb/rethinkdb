#include "clustering/dispatching_store.hpp"
#include "concurrency/pmap.hpp"

dispatching_store_t::dispatching_store_t() {
}

dispatching_store_t::~dispatching_store_t() {
    rassert(dispatchees.empty());
}

static void do_mutation(int i, mutation_result_t *out, std::pair<set_store_t *, get_store_t *> s, const mutation_t &mut, castime_t castime) {
    /* The reply that we send back to the client is the reply produced by the first dispatchee. */
    if (i == 0) {
        *out = s.first->change(mut, castime);
    } else {
        mutation_result_t r = s.first->change(mut, castime);
        /* Special case: If it was a get_cas, we need to signal the cond to indicate we are throwing
        the result away. */
        if (get_result_t *gr = boost::get<get_result_t>(&r.result)) {
            if (gr->to_signal_when_done) gr->to_signal_when_done->pulse();
        }
    }
}

mutation_result_t dispatching_store_t::change(const mutation_t &mut, castime_t castime) {
    rassert(!dispatchees.empty());
    mutation_result_t res;
    pimap(dispatchees.get_storage(mut.get_key()), dispatchees.end(), boost::bind(&do_mutation, _2, &res, _1, mut, castime));
    return res;
}

get_result_t dispatching_store_t::get(const store_key_t &key) {
    rassert(!dispatchees.empty());
    return (*dispatchees.get_storage(key)).second->get(key);
}

dispatching_store_t::dispatchee_t::dispatchee_t(int bucket, dispatching_store_t *p, std::pair<set_store_t *, get_store_t *> c)
    : client(c), parent(p), bucket(bucket) {
    parent->dispatchees.add_storage(bucket, c);
}

dispatching_store_t::dispatchee_t::~dispatchee_t() {
    parent->dispatchees.remove_storage(bucket);
}
