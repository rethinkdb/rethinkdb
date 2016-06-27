// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "random.hpp"

#include "containers/scoped.hpp"
#include "thread_local.hpp"

rng_t::rng_t()
    : m_mt19937(std::random_device{}()) {
}

rng_t::rng_t(uint64_t seed)
    : m_mt19937(seed) {
}

int rng_t::randint(int n) {
    assert_thread();
    guarantee(n > 0, "non-positive argument for randint's [0, n) interval");

    // `std::uniform_int_distribution` operates on [a, b]
    return std::uniform_int_distribution<int>(0, n - 1)(m_mt19937);
}

uint64_t rng_t::randuint64(uint64_t n) {
    assert_thread();
    guarantee(n > 0, "non-positive argument for randint's [0, n) interval");

    // `std::uniform_int_distribution` operates on [a, b]
    return std::uniform_int_distribution<uint64_t>(0, n - 1)(m_mt19937);
}

size_t rng_t::randsize(size_t n) {
    assert_thread();
    guarantee(n > 0, "non-positive argument for randint's [0, n) interval");

    // `std::uniform_int_distribution` operates on [a, b]
    return std::uniform_int_distribution<size_t>(0, n - 1)(m_mt19937);
}

double rng_t::randdouble() {
    assert_thread();
    return std::uniform_real_distribution<double>(0, 1)(m_mt19937);
}

TLS_with_init(rng_t*, rng, nullptr)

rng_t *get_TLS_rng() {
    // This lazy construction is to ensure that we construct the thread local
    // rng_t on the correct thread, instead of creating it on startup.

    if (TLS_get_rng() == nullptr) {
        TLS_set_rng(new rng_t());
    }
    return TLS_get_rng();
}

int randint(int n) {
    return get_TLS_rng()->randint(n);
}

uint64_t randuint64(uint64_t n) {
    return get_TLS_rng()->randuint64(n);
}

size_t randsize(size_t n) {
    return get_TLS_rng()->randsize(n);
}

double randdouble() {
    return get_TLS_rng()->randdouble();
}
