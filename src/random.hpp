// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

#include "errors.hpp"

#include "threading.hpp"

class rng_t : public home_thread_mixin_debug_only_t {
public:
    rng_t();
    explicit rng_t(uint64_t seed);

    // A uniform random number in [0, n).
    int randint(int n);
    uint64_t randuint64(uint64_t n);
    size_t randsize(size_t n);

    double randdouble();

private:
    std::mt19937_64 m_mt19937;

    DISABLE_COPYING(rng_t);
};

int randint(int n);
uint64_t randuint64(uint64_t n);
size_t randsize(size_t n);

double randdouble();

#endif // RANDOM_HPP_
