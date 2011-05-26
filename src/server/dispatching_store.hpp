#ifndef __SERVER_DISPATCHING_STORE_HPP__
#define __SERVER_DISPATCHING_STORE_HPP__

#include "store.hpp"
#include "concurrency/fifo_checker.hpp"

struct dispatching_store_t : public set_store_t, public home_thread_mixin_t {

    dispatching_store_t(set_store_t *substore);
    void set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t, order_token_t)> disp);

    mutation_result_t change(const mutation_t &m, castime_t castime, order_token_t token);

private:
    order_source_t order_source_for_dispatchee;
    boost::function<mutation_t(const mutation_t&, castime_t, order_token_t)> dispatcher;
    set_store_t *substore;
};

#endif /* __SERVER_DISPATCHING_STORE_HPP__ */
