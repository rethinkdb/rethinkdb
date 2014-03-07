/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <string>
#include <boost/detail/lightweight_test.hpp>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>

using namespace phoenix;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    int     i1 = 1, i2 = 2, i50 = 50, i100 = 100;
    double  d2_5 = 2.5;
    string hello = "hello";
    const char* world = " world";

///////////////////////////////////////////////////////////////////////////////
//
//  Mixed type operators
//
///////////////////////////////////////////////////////////////////////////////
    BOOST_TEST((arg1 + arg2)(i100, i50) == (i100 + i50));
    BOOST_TEST((arg1 + 3)(i100) == (3 + i100));
    BOOST_TEST((arg1 + arg2)(hello, world) == "hello world");
    BOOST_TEST((arg1 + arg2)(i1, d2_5) == (i1 + d2_5));

    BOOST_TEST((*(arg1 + arg2))(world, i2) == *(world + i2));
    BOOST_TEST((*(arg1 + arg2))(i2, world) == *(i2 + world));
    BOOST_TEST((*(val(world+i2) - arg1))(i2) == *world);

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();    
}
