#include "server/dispatching_store.hpp"

dispatching_store_t::dispatching_store_t(set_store_t *ss, int bucket) : order_source_for_dispatchee(bucket), substore(ss) { }

void dispatching_store_t::set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t, order_token_t)> disp) {
    assert_thread();
    dispatcher = disp;
}

mutation_result_t dispatching_store_t::change(const mutation_t &m, castime_t castime, order_token_t substore_token) {
    assert_thread();
    if (dispatcher) {
        order_token_t dispatchee_token = order_source_for_dispatchee.check_in();
        return substore->change(dispatcher(m, castime, dispatchee_token), castime, substore_token);
    } else {
        return substore->change(m, castime, substore_token);
    }
}
