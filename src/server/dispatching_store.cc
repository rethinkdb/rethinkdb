#include "server/dispatching_store.hpp"

dispatching_store_t::dispatching_store_t(set_store_t *ss) : substore(ss) { }

void dispatching_store_t::set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t)> disp) {
    dispatcher = disp;
}

mutation_result_t dispatching_store_t::change(const mutation_t &m, castime_t castime) {
    if (dispatcher) {
        return substore->change(dispatcher(m, castime), castime);
    } else {
        return substore->change(m, castime);
    }
}