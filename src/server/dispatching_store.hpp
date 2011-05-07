#ifndef __SERVER_DISPATCHING_STORE_HPP__
#define __SERVER_DISPATCHING_STORE_HPP__

#include "store.hpp"

struct dispatching_store_t : public set_store_t {

    dispatching_store_t(set_store_t *substore);
    void set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t)> disp);

    mutation_result_t change(const mutation_t &m, castime_t castime);

private:
    boost::function<mutation_t(const mutation_t &, castime_t)> dispatcher;
    set_store_t *substore;
};

#endif /* __SERVER_DISPATCHING_STORE_HPP__ */
