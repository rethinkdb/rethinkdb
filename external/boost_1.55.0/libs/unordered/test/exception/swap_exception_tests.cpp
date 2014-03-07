
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./containers.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/invariants.hpp"

#if defined(BOOST_MSVC)
#pragma warning(disable:4512) // assignment operator could not be generated
#endif

test::seed_t initialize_seed(9387);

template <class T>
struct self_swap_base : public test::exception_base
{
    test::random_values<T> values;
    self_swap_base(int count = 0) : values(count) {}

    typedef T data_type;
    T init() const { return T(values.begin(), values.end()); }
    void run(T& x) const { x.swap(x); }
    void check BOOST_PREVENT_MACRO_SUBSTITUTION(T const& x) const {
        std::string scope(test::scope);

        // TODO: In C++11 exceptions are only allowed in the swap function.
        BOOST_TEST(
                scope == "hash::hash(hash)" ||
                scope == "hash::operator=(hash)" ||
                scope == "equal_to::equal_to(equal_to)" ||
                scope == "equal_to::operator=(equal_to)");

        test::check_equivalent_keys(x);
    }
};

template <class T>
struct self_swap_test1 : self_swap_base<T> {};

template <class T>
struct self_swap_test2 : self_swap_base<T>
{
    self_swap_test2() : self_swap_base<T>(100) {}
};

template <class T>
struct swap_base : public test::exception_base
{
    const test::random_values<T> x_values, y_values;
    const T initial_x, initial_y;

    typedef BOOST_DEDUCED_TYPENAME T::hasher hasher;
    typedef BOOST_DEDUCED_TYPENAME T::key_equal key_equal;
    typedef BOOST_DEDUCED_TYPENAME T::allocator_type allocator_type;

    swap_base(unsigned int count1, unsigned int count2, int tag1, int tag2)
        : x_values(count1), y_values(count2),
        initial_x(x_values.begin(), x_values.end(), 0, hasher(tag1),
                key_equal(tag1), allocator_type(tag1)),
        initial_y(y_values.begin(), y_values.end(), 0, hasher(tag2),
                key_equal(tag2), allocator_type(
                    T::allocator_type::propagate_on_container_swap::value ?
                        tag2 : tag1))
    {}

    struct data_type {
        data_type(T const& x, T const& y)
            : x(x), y(y) {}

        T x, y;
    };

    data_type init() const { return data_type(initial_x, initial_y); }

    void run(data_type& d) const {
        try {
            d.x.swap(d.y);
        } catch (std::runtime_error) {}
    }
    void check BOOST_PREVENT_MACRO_SUBSTITUTION(data_type const& d) const {
        std::string scope(test::scope);

        // TODO: In C++11 exceptions are only allowed in the swap function.
        BOOST_TEST(
                scope == "hash::hash(hash)" ||
                scope == "hash::operator=(hash)" ||
                scope == "equal_to::equal_to(equal_to)" ||
                scope == "equal_to::operator=(equal_to)");
        
        test::check_equivalent_keys(d.x);
        test::check_equivalent_keys(d.y);
    }
};

template <class T>
struct swap_test1 : swap_base<T>
{
    swap_test1() : swap_base<T>(0, 0, 0, 0) {}
};

template <class T>
struct swap_test2 : swap_base<T>
{
    swap_test2() : swap_base<T>(60, 0, 0, 0) {}    
};

template <class T>
struct swap_test3 : swap_base<T>
{
    swap_test3() : swap_base<T>(0, 60, 0, 0) {}    
};

template <class T>
struct swap_test4 : swap_base<T>
{
    swap_test4() : swap_base<T>(10, 10, 1, 2) {}
};

EXCEPTION_TESTS(
    (self_swap_test1)(self_swap_test2)
    (swap_test1)(swap_test2)(swap_test3)(swap_test4),
    CONTAINER_SEQ)
RUN_TESTS()
