
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./containers.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/invariants.hpp"

#if defined(BOOST_MSVC)
#pragma warning(disable:4512) // assignment operator could not be generated
#endif

test::seed_t initialize_seed(12847);

template <class T>
struct self_assign_base : public test::exception_base
{
    test::random_values<T> values;
    self_assign_base(int count = 0) : values(count) {}

    typedef T data_type;
    T init() const { return T(values.begin(), values.end()); }
    void run(T& x) const { x = x; }
    void check BOOST_PREVENT_MACRO_SUBSTITUTION(T const& x) const
        { test::check_equivalent_keys(x); }
};

template <class T>
struct self_assign_test1 : self_assign_base<T> {};

template <class T>
struct self_assign_test2 : self_assign_base<T>
{
    self_assign_test2() : self_assign_base<T>(100) {}
};

template <class T>
struct assign_base : public test::exception_base
{
    const test::random_values<T> x_values, y_values;
    T x,y;

    typedef BOOST_DEDUCED_TYPENAME T::hasher hasher;
    typedef BOOST_DEDUCED_TYPENAME T::key_equal key_equal;
    typedef BOOST_DEDUCED_TYPENAME T::allocator_type allocator_type;

    assign_base(unsigned int count1, unsigned int count2, int tag1, int tag2,
        float mlf1 = 1.0, float mlf2 = 1.0) :
        x_values(count1),
        y_values(count2),
        x(x_values.begin(), x_values.end(), 0, hasher(tag1), key_equal(tag1),
            allocator_type(tag1)),
        y(y_values.begin(), y_values.end(), 0, hasher(tag2), key_equal(tag2),
            allocator_type(tag2))
    {
        x.max_load_factor(mlf1);
        y.max_load_factor(mlf2);
    }

    typedef T data_type;
    T init() const { return T(x); }
    void run(T& x1) const { x1 = y; }
    void check BOOST_PREVENT_MACRO_SUBSTITUTION(T const& x1) const
    {
        test::check_equivalent_keys(x1);

        // If the container is empty at the point of the exception, the
        // internal structure is hidden, this exposes it.
        T& y = const_cast<T&>(x1);
        if (x_values.size()) {
            y.emplace(*x_values.begin());
            test::check_equivalent_keys(y);
        }
    }
};

template <class T>
struct assign_test1 : assign_base<T>
{
    assign_test1() : assign_base<T>(0, 0, 0, 0) {}
};

template <class T>
struct assign_test2 : assign_base<T>
{
    assign_test2() : assign_base<T>(60, 0, 0, 0) {}    
};

template <class T>
struct assign_test3 : assign_base<T>
{
    assign_test3() : assign_base<T>(0, 60, 0, 0) {}    
};

template <class T>
struct assign_test4 : assign_base<T>
{
    assign_test4() : assign_base<T>(10, 10, 1, 2) {}
};

template <class T>
struct assign_test5 : assign_base<T>
{
    assign_test5() : assign_base<T>(5, 60, 0, 0, 1.0, 0.1) {}
};

EXCEPTION_TESTS(
    (self_assign_test1)(self_assign_test2)
    (assign_test1)(assign_test2)(assign_test3)(assign_test4)(assign_test5),
    CONTAINER_SEQ)
RUN_TESTS()
