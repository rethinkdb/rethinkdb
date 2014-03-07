/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <boost/noncopyable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;
namespace phx = boost::phoenix;

namespace test
{
    struct x : boost::noncopyable // test non-copyable (hold this by reference)
    {
        int m;
    };

    struct xx {
       int m;
    };
}

template<class T, class F>
void write_test(F f) {
    T x_;
    bind(&T::m, f(x_))() = 122;
    BOOST_TEST(x_.m == 122);
    bind(&T::m, arg1)(f(x_)) = 123;
    BOOST_TEST(x_.m == 123);
}

template<class T, class F>
void read_test(F f) {
    T x_;
    x_.m = 123;

    BOOST_TEST(bind(&T::m, f(x_))() == 123);
    BOOST_TEST(bind(&T::m, arg1)(f(x_)) == 123);
}

struct identity
{
    template<class T>
    T& operator()(T& t) const
    {
        return t;
    }
};

struct constify
{
    template<class T>
    const T& operator()(const T& t) const
    {
        return t;
    }
};

struct add_pointer
{
    template<class T>
    T* const operator()(T& t) const
    {
        return &t;
    }
};

struct add_const_pointer
{
    template<class T>
    const T* const operator()(const T& t) const
    {
        return &t;
    }
};

int
main()
{
    write_test<test::x>(::identity());
    write_test<test::x>(::add_pointer());
    write_test<test::xx>(::identity());
    write_test<test::xx>(::add_pointer());

    read_test<test::x>(::identity());
    //read_test<test::x>(constify()); // this fails because of capture by value.
    read_test<test::x>(::add_pointer());
    read_test<test::x>(add_const_pointer());
    read_test<test::xx>(::identity());
    //read_test<test::xx>(constify());// this fails because of capture by value.
    read_test<test::xx>(::add_pointer());
    read_test<test::xx>(add_const_pointer());
    return boost::report_errors();
}
