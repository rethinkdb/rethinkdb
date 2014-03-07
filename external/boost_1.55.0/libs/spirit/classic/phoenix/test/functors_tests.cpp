/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <boost/detail/lightweight_test.hpp>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>

using namespace phoenix;
using namespace std;

    ///////////////////////////////////////////////////////////////////////////////
    struct test_ {

        typedef void result_type;
        void operator()() const { cout << "TEST LAZY FUNCTION\n"; }
    };

    function<test_> test;

    ///////////////////////////////////////////////////////////////////////////////
    struct sqr_ {

        template <typename ArgT>
        struct result { typedef ArgT type; };

        template <typename ArgT>
        ArgT operator()(ArgT n) const { return n * n; }
    };

    function<sqr_> sqr;

    ///////////////////////////////////////////////////////////////////////////////
    struct fact_ {

        template <typename ArgT>
        struct result { typedef ArgT type; };

        template <typename ArgT>
        ArgT operator()(ArgT n) const
        { return (n <= 0) ? 1 : n * this->operator()(n-1); }
    };

    function<fact_> fact;

    ///////////////////////////////////////////////////////////////////////////////
    struct pow_ {

        template <typename Arg1T, typename Arg2T>
        struct result { typedef Arg1T type; };

        template <typename Arg1T, typename Arg2T>
        Arg1T operator()(Arg1T a, Arg2T b) const { return pow(a, b); }
    };

    function<pow_> power;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    int     i5 = 5;
    double  d5 = 5, d3 = 3;

///////////////////////////////////////////////////////////////////////////////
//
//  Lazy functors
//
///////////////////////////////////////////////////////////////////////////////

    test()();
    BOOST_TEST(sqr(arg1)(i5) == (i5*i5));
    BOOST_TEST(fact(4)() == 24);
    BOOST_TEST(fact(arg1)(i5) == 120);    
    BOOST_TEST((int)power(arg1, arg2)(d5, d3) == (int)pow(d5, d3));
    BOOST_TEST((sqr(arg1) + 5)(i5) == ((i5*i5)+5));

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();
}
