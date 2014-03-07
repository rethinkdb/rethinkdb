
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./containers.hpp"
#include <string>
#include "../helpers/random_values.hpp"
#include "../helpers/invariants.hpp"
#include "../helpers/strong.hpp"

#include <iostream>

test::seed_t initialize_seed(3298597);

template <class T>
struct rehash_test_base : public test::exception_base
{
    test::random_values<T> values;
    unsigned int n;
    rehash_test_base(unsigned int count = 100, unsigned int n = 0)
        : values(count), n(n)
    {}

    typedef T data_type;
    typedef test::strong<T> strong_type;

    data_type init() const {
        T x(values.begin(), values.end(), n);
        return x;
    }

    void check BOOST_PREVENT_MACRO_SUBSTITUTION(T const& x,
        strong_type const& strong) const
    {
        std::string scope(test::scope);

        if(scope.find("hash::operator()") == std::string::npos &&
                scope.find("equal_to::operator()") == std::string::npos &&
                scope != "operator==(object, object)")
            strong.test(x);

        test::check_equivalent_keys(x);
    }
};

template <class T>
struct rehash_test0 : rehash_test_base<T>
{
    rehash_test0() : rehash_test_base<T>(0) {}
    void run(T& x) const { x.rehash(0); }
};

template <class T>
struct rehash_test1 : rehash_test_base<T>
{
    rehash_test1() : rehash_test_base<T>(0) {}
    void run(T& x) const { x.rehash(200); }
};

template <class T>
struct rehash_test2 : rehash_test_base<T>
{
    rehash_test2() : rehash_test_base<T>(0, 200) {}
    void run(T& x) const { x.rehash(0); }
};

template <class T>
struct rehash_test3 : rehash_test_base<T>
{
    rehash_test3() : rehash_test_base<T>(10, 0) {}
    void run(T& x) const { x.rehash(200); }
};

template <class T>
struct rehash_test4 : rehash_test_base<T>
{
    rehash_test4() : rehash_test_base<T>(10, 200) {}
    void run(T& x) const { x.rehash(0); }
};

EXCEPTION_TESTS(
    (rehash_test0)(rehash_test1)(rehash_test2)(rehash_test3)(rehash_test4),
    CONTAINER_SEQ)
RUN_TESTS()
