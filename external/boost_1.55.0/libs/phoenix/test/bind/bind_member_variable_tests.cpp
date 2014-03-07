/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <boost/noncopyable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>

namespace test
{
    struct x //: boost::noncopyable // test non-copyable (hold this by reference)
    {
        int m;
    };

    struct xx {
       int m;
    };
}

template <typename T, typename F>
void
write_test(F f)
{
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::bind;

    T x_;

    bind(&T::m, f(x_))() = 122;
    BOOST_TEST(x_.m == 122);
    bind(&T::m, arg1)(f(x_)) = 123;
    BOOST_TEST(x_.m == 123);
}

template <typename T, typename F>
void
read_test(F f)
{
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::bind;

    T x_;
    x_.m = 123;

    BOOST_TEST(bind(&T::m, f(x_))() == 123);
    BOOST_TEST(bind(&T::m, arg1)(f(x_)) == 123);
}

struct identity
{
    template <typename T>
    T&
    operator()(T& t) const
    {
        return t;
    }
};

struct constify
{
    template <typename T>
    T const&
    operator()(T const& t) const
    {
        return t;
    }
};

struct add_pointer
{
    template <typename T>
    T* const
    operator()(T& t) const
    {
        return &t;
    }
};

struct add_const_pointer
{
    template <typename T>
    const T* const
    operator()(T const& t) const
    {
        return &t;
    }
};

int
main()
{
    write_test<test::x>(add_pointer());
    write_test<test::xx>(add_pointer());

    read_test<test::x>(identity());
    read_test<test::x>(constify());
    read_test<test::x>(add_pointer());
    read_test<test::x>(add_const_pointer());
    read_test<test::xx>(identity());
    read_test<test::xx>(constify());
    read_test<test::xx>(add_pointer());
    read_test<test::xx>(add_const_pointer());

    return boost::report_errors();
}
