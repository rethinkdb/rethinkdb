// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PMAP_HPP_
#define CONCURRENCY_PMAP_HPP_

#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/new_semaphore.hpp"

template <class callable_t, class value_t>
struct pmap_runner_one_arg_t {
    value_t i;
    const callable_t *c;
    int64_t *outstanding;
    cond_t *to_signal;
    pmap_runner_one_arg_t(value_t _i, const callable_t *_c, int64_t *_outstanding,
                          cond_t *_to_signal)
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
void spawn_pmap_runner_one_arg(value_t i, const callable_t *c, int64_t *outstanding, cond_t *to_signal) {
    coro_t::spawn_now_dangerously(pmap_runner_one_arg_t<callable_t, value_t>(i, c, outstanding, to_signal));
}

template <class callable_t>
void pmap(int64_t begin, int64_t end, const callable_t &c) {
    guarantee(begin >= 0);  // We don't want `end - begin` to overflow, do we?
    guarantee(begin <= end);
    if (begin == end) {
        return;
    }

    cond_t cond;
    int64_t outstanding = (end - begin);
    for (int64_t i = begin; i < end - 1; ++i) {
        coro_t::spawn_now_dangerously(pmap_runner_one_arg_t<callable_t, int64_t>(i, &c, &outstanding, &cond));
    }
    pmap_runner_one_arg_t<callable_t, int64_t> runner(end - 1, &c, &outstanding, &cond);
    runner();
    cond.wait();
}

template <class callable_t>
void pmap(int64_t count, const callable_t &c) {
    pmap(0, count, c);
}

template <class callable_t, class iterator_t>
void pmap(iterator_t start, iterator_t end, const callable_t &c) {
    cond_t cond;
    int64_t outstanding = 1;
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

template <class callable_t, class value_t>
struct throttled_pmap_runner_t {
    value_t i;
    const callable_t *c;
    int64_t *outstanding;
    cond_t *to_signal;
    new_semaphore_acq_t semaphore_acq;
    throttled_pmap_runner_t(value_t _i, const callable_t *_c, int64_t *_outstanding,
                            cond_t *_to_signal, new_semaphore_acq_t &&acq)
        : i(_i), c(_c), outstanding(_outstanding), to_signal(_to_signal),
          semaphore_acq(std::move(acq)) { }

    void operator()() {
        (*c)(i);
        (*outstanding)--;
        semaphore_acq.reset();
        if (*outstanding == 0) {
            to_signal->pulse();
        }
    }
};

template <class callable_t>
void throttled_pmap(int64_t begin, int64_t end, const callable_t &c, int64_t capacity) {
    guarantee(capacity > 0);
    guarantee(begin >= 0);  // We don't want `end - begin` to overflow, do we?
    guarantee(begin <= end);
    if (begin == end) {
        return;
    }

    new_semaphore_t semaphore(capacity);
    cond_t cond;
    int64_t outstanding = (end - begin);
    for (int64_t i = begin; i < end - 1; ++i) {
        new_semaphore_acq_t acq(&semaphore, 1);
        acq.acquisition_signal()->wait();
        coro_t::spawn_now_dangerously(
            throttled_pmap_runner_t<callable_t, int64_t>(i, &c, &outstanding, &cond,
                                                         std::move(acq)));
    }

    {
        new_semaphore_acq_t acq(&semaphore, 1);
        acq.acquisition_signal()->wait();
        throttled_pmap_runner_t<callable_t, int64_t> runner(end - 1, &c, &outstanding,
                                                        &cond, std::move(acq));
        runner();
    }
    cond.wait();
}

template <class callable_t>
void throttled_pmap(int64_t count, const callable_t &c, int64_t capacity) {
    throttled_pmap(0, count, c, capacity);
}

#endif /* CONCURRENCY_PMAP_HPP_ */
