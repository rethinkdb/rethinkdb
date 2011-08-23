
#ifndef __CONCURRENCY_PMAP_HPP__
#define __CONCURRENCY_PMAP_HPP__

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"

template<typename callable_t, typename value_t>
void pmap_runner_one_arg(value_t i, const callable_t *c, int *outstanding, cond_t *to_signal) {
    (*c)(i);
    (*outstanding)--;
    if (*outstanding == 0) {
	to_signal->pulse();
    }
}


template<typename callable_t, typename value_t>
void spawn_pmap_runner_one_arg(value_t i, const callable_t *c, int *outstanding, cond_t *to_signal) {
    coro_t::spawn_now(boost::bind(&pmap_runner_one_arg<callable_t, value_t>, i, c, outstanding, to_signal));
}

template<typename callable_t>
void pmap(int count, const callable_t &c) {
    if (count == 0) {
	return;
    }
    if (count == 1) {
        c(0);
        return;
    }

    cond_t cond;
    int outstanding = count - 1;
    for (int i = 0; i < count - 1; i++) {
        coro_t::spawn_now(boost::bind(&pmap_runner_one_arg<callable_t, int>, i, &c, &outstanding, &cond));
    }
    c(count - 1);
    cond.wait();
}

// TODO: Passing end by reference seems very questionable to me.
template<typename callable_t, typename iterator_t>
void pmap(iterator_t start, const iterator_t &end, const callable_t &c) {
    cond_t cond;
    int outstanding = 1;
    while (start != end) {
        outstanding++;
        spawn_pmap_runner_one_arg(*start, &c, &outstanding, &cond);
        start++;
    }
    outstanding--;
    if (outstanding) {
	cond.wait();
    }
}

template<typename callable_t, typename value1_t, typename value2_t>
void pmap_runner_two_arg(value1_t i, value2_t i2, const callable_t *c, int *outstanding, cond_t *to_signal) {
    (*c)(i, i2);
    (*outstanding)--;
    if (*outstanding == 0) {
	to_signal->pulse();
    }
}

template<typename callable_t, typename value1_t, typename value2_t>
void spawn_pmap_runner_two_arg(value1_t i, value2_t i2, const callable_t *c, int *outstanding, cond_t *to_signal) {
    coro_t::spawn_now(boost::bind(&pmap_runner_two_arg<callable_t, value1_t, value2_t>, i, i2, c, outstanding, to_signal));
}

template<typename callable_t, typename iterator_t>
void pimap(iterator_t start, const iterator_t &end, const callable_t &c) {
    cond_t cond;
    int outstanding = 1;
    int i = 0;
    while (start != end) {
        outstanding++;
        spawn_pmap_runner_two_arg(*start, i, &c, &outstanding, &cond);
        i++;
        start++;
    }
    outstanding--;
    if (outstanding) {
	cond.wait();
    }
}

#endif /* __CONCURRENCY_PMAP_HPP__ */
