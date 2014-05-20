// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PMAP_HPP_
#define CONCURRENCY_PMAP_HPP_

#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"

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
void pmap(int begin, int end, const callable_t &c) {
    guarantee(begin >= 0);  // We don't want `end - begin` to overflow, do we?
    guarantee(begin <= end);
    if (begin == end) {
        return;
    }

    cond_t cond;
    int outstanding = (end - begin);
    for (int i = begin; i < end - 1; ++i) {
        coro_t::spawn_now_dangerously(pmap_runner_one_arg_t<callable_t, int>(i, &c, &outstanding, &cond));
    }
    pmap_runner_one_arg_t<callable_t, int> runner(end - 1, &c, &outstanding, &cond);
    runner();
    cond.wait();
}

template <class callable_t>
void pmap(int count, const callable_t &c) {
    pmap(0, count, c);
}

template <class callable_t, class iterator_t>
void pmap(iterator_t start, iterator_t end, const callable_t &c) {
    cond_t cond;
    int outstanding = 1;
    while (start != end) {
        ++outstanding;
        spawn_pmap_runner_one_arg(*start, &c, &outstanding, &cond);
        ++start;
    }
    --outstanding;
    if (outstanding) {
        cond.wait();
    }
}

#endif /* CONCURRENCY_PMAP_HPP_ */
