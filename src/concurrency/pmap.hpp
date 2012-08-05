#ifndef CONCURRENCY_PMAP_HPP_
#define CONCURRENCY_PMAP_HPP_

#include "arch/runtime/runtime.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"

template <class callable_t, class value_t>
struct pmap_runner_one_arg_t {
    value_t i;
    const callable_t *c;
    int *outstanding;
    cond_t *to_signal;
    pmap_runner_one_arg_t(value_t _i, const callable_t *_c, int *_outstanding, cond_t *_to_signal)
        : i(_i), c(_c), outstanding(_outstanding), to_signal(_to_signal) { }

    void operator()() {
        (*c)(i);
        (*outstanding)--;
        if (*outstanding == 0) {
            to_signal->pulse();
        }
    }
};

template <class callable_t, class value_t>
void spawn_pmap_runner_one_arg(value_t i, const callable_t *c, int *outstanding, cond_t *to_signal) {
    coro_t::spawn_now_dangerously(pmap_runner_one_arg_t<callable_t, value_t>(i, c, outstanding, to_signal));
}

template <class callable_t>
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
        coro_t::spawn_now_dangerously(pmap_runner_one_arg_t<callable_t, int>(i, &c, &outstanding, &cond));
    }
    c(count - 1);
    cond.wait();
}

template <class callable_t, class iterator_t>
void pmap(iterator_t start, iterator_t end, const callable_t &c) {
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

template <class callable_t, class value1_t, class value2_t>
class pmap_runner_two_arg_t {
    value1_t i;
    value2_t i2;
    const callable_t *c;
    int *outstanding;
    cond_t *to_signal;

    pmap_runner_two_arg_t(value1_t _i, value2_t _i2, const callable_t *_c, int *_outstanding, cond_t *_to_signal)
        : i(_i), i2(_i2), c(_c), outstanding(_outstanding), to_signal(_to_signal) { }

    void operator()() {
        (*c)(i, i2);
        (*outstanding)--;
        if (*outstanding == 0) {
            to_signal->pulse();
        }
    }
};

template <class callable_t, class value1_t, class value2_t>
void spawn_pmap_runner_two_arg(value1_t i, value2_t i2, const callable_t *c, int *outstanding, cond_t *to_signal) {
    coro_t::spawn_now_dangerously(pmap_runner_two_arg_t<callable_t, value1_t, value2_t>(i, i2, c, outstanding, to_signal));
}

template <class callable_t, class iterator_t>
void pimap(iterator_t start, iterator_t end, const callable_t &c) {
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

#endif /* CONCURRENCY_PMAP_HPP_ */
