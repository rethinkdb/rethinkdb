// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

#include "errors.hpp"

class rng_t {
public:
    rng_t();
    rng_t(uint64_t seed);

    // A uniform random number in [0, n).
    int randint(int n);
    uint64_t randuint64(uint64_t n);
    size_t randsize(size_t n);

    double randdouble();

private:
    std::ranlux48 m_ranlux48;

    DISABLE_COPYING(rng_t);
};

int randint(int n);
uint64_t randuint64(uint64_t n);
size_t randsize(size_t n);

double randdouble();

#endif // RANDOM_HPP_
