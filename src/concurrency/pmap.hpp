
#ifndef __CONCURRENCY_PMAP_HPP__
#define __CONCURRENCY_PMAP_HPP__

#include "concurrency/cond_var.hpp"
#include "errors.hpp"
#include <boost/function.hpp>

template<typename callable_t>
void pmap_one(int i, const callable_t *c, int *outstanding, cond_t *to_signal) {
    (*c)(i);
    (*outstanding)--;
    if (*outstanding == 0) to_signal->pulse();
}

template<typename callable_t>
void pmap(int count, const callable_t &c) {
    if (count == 0) return;
    if (count == 1) {
        c(0);
        return;
    }

    cond_t cond;
    int outstanding = count - 1;
    for (int i = 0; i < count - 1; i++) {
        coro_t::spawn_now(boost::bind(&pmap_one<callable_t>, i, &c, &outstanding, &cond));
    }
    c(count - 1);
    cond.wait();
}

#endif /* __CONCURRENCY_PMAP_HPP__ */
