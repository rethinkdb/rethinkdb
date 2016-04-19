// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "random.hpp"

#include "thread_local.hpp"

rng_t::rng_t()
    : m_ranlux48(std::random_device{}()) {
}

rng_t::rng_t(uint64_t seed)
    : m_ranlux48(seed) {
}

int rng_t::randint(int n) {
    assert_thread();
    guarantee(n > 0, "non-positive argument for randint's [0, n) interval");

    // `std::uniform_int_distribution` operates on [a, b]
    return std::uniform_int_distribution<int>(0, n - 1)(m_ranlux48);
}

uint64_t rng_t::randuint64(uint64_t n) {
    assert_thread();
    guarantee(n > 0, "non-positive argument for randint's [0, n) interval");

    // `std::uniform_int_distribution` operates on [a, b]
    return std::uniform_int_distribution<uint64_t>(0, n - 1)(m_ranlux48);
}

size_t rng_t::randsize(size_t n) {
    assert_thread();
    guarantee(n > 0, "non-positive argument for randint's [0, n) interval");

    // `std::uniform_int_distribution` operates on [a, b]
    return std::uniform_int_distribution<size_t>(0, n - 1)(m_ranlux48);
}

double rng_t::randdouble() {
    assert_thread();
    return std::uniform_real_distribution<double>(0, 1)(m_ranlux48);
}

TLS_ptr_with_constructor(rng_t, rng)

int randint(int n) {
    return TLS_ptr_rng()->randint(n);
}

uint64_t randuint64(uint64_t n) {
    return TLS_ptr_rng()->randuint64(n);
}

size_t randsize(size_t n) {
    return TLS_ptr_rng()->randsize(n);
}

double randdouble() {
    return TLS_ptr_rng()->randdouble();
}
