/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>

using namespace phoenix;
using namespace std;

    ///////////////////////////////////////////////////////////////////////////////
    struct sqr_ {

        template <typename ArgT>
        struct result { typedef ArgT type; };

        template <typename ArgT>
        ArgT operator()(ArgT n) const { return n * n; }
    };

    function<sqr_> sqr;

    ///////////////////////////////////////////////////////////////////////////////
    struct adder_ {

        template <typename Arg1T, typename Arg2T, typename ArgT3>
        struct result { typedef Arg1T type; };

        template <typename Arg1T, typename Arg2T, typename ArgT3>
        Arg1T operator()(Arg1T a, Arg2T b, ArgT3 c) const { return a + b + c; }
    };

    function<adder_> adder;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    int     i2 = 2, i = 4, i50 = 50, i10 = 10, i20 = 20, i100 = 100;
    double  d5 = 5, d10 = 10;
    string hello = "hello";

///////////////////////////////////////////////////////////////////////////////
//
//  More complex expressions
//
///////////////////////////////////////////////////////////////////////////////
    BOOST_TEST((10 - arg1)(i100) == (10 - i100));
    BOOST_TEST((20 - arg1)(i100) == (20 - i100));
    BOOST_TEST((arg1 - 10)(i100) == (i100 - 10));
    BOOST_TEST((arg1 - 20)(i100) == (i100 - 20));
    BOOST_TEST((arg1 - arg2)(i100, i50) == (i100 - i50));
    BOOST_TEST((arg1 - var(i))(i10) == (i10 - i));
    BOOST_TEST((arg1 + arg2 - arg3)(i100, i50, i20) == (i100 + i50 - i20));
    BOOST_TEST((sqr(arg1) + arg2 - arg3)(i100, i50, i20) == ((i100*i100) + i50 - i20));

    int ii = i;
    BOOST_TEST((var(i) += arg1)(i2) == (ii += i2));
    BOOST_TEST((sqr(sqr(arg1)))(i100) == (i100*i100*i100*i100));


#if 0   /*** SHOULD NOT COMPILE ***/
    (val(3) += arg1)(i100);
    (val(3) = 3)();
#endif

    BOOST_TEST(((adder(arg1, arg2, arg3) + arg2 - arg3)(i100, i50, i20)) == (i100 + i50 + i20) + i50 - i20);
    BOOST_TEST((adder(arg1, arg2, arg3)(i100, i50, i20)) == (i100 + i50 + i20));
    BOOST_TEST((sqr(sqr(sqr(sqr(arg1)))))(d10) == 1e16);
    BOOST_TEST((sqr(sqr(arg1)) / arg1 / arg1)(d5) == 25);

    for (int j = 0; j < 20; ++j)
    {
        cout << (10 < arg1)(j);
        BOOST_TEST((10 < arg1)(j) == (10 < j));
    }
    cout << endl;

    for (int k = 0; k < 20; ++k)
    {
        bool r = ((arg1 % 2 == 0) && (arg1 < 15))(k);
        cout << r;
        BOOST_TEST(r == ((k % 2 == 0) && (k < 15)));
    }
    cout << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();    
}
