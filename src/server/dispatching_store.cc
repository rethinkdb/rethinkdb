#include "server/dispatching_store.hpp"

dispatching_store_t::dispatching_store_t(set_store_t *ss, int bucket) : order_source(bucket), substore(ss) { }

void dispatching_store_t::set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t, order_token_t)> disp) {
    assert_thread();
    dispatcher = disp;
}

mutation_result_t dispatching_store_t::change(const mutation_t &m, castime_t castime, order_token_t key_value_store_token) {
    assert_thread();
    order_sink.check_out(key_value_store_token);
    order_token_t token = order_source.check_in();
    if (dispatcher) {
        return substore->change(dispatcher(m, castime, token), castime, token);
    } else {
        return substore->change(m, castime, token);
    }
}
