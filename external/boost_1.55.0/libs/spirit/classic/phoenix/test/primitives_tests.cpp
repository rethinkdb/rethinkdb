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

using namespace phoenix;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    char c1 = '1';
    int i1 = 1, i2 = 2, i = 4;
    const char* s2 = "2";

///////////////////////////////////////////////////////////////////////////////
//
//  Values, variables and arguments
//
///////////////////////////////////////////////////////////////////////////////
    cout << val("Hello")() << val(' ')() << val("World")() << endl;

    BOOST_TEST(arg1(c1) == c1);
    BOOST_TEST(arg1(i1, i2) == i1);
    BOOST_TEST(arg2(i1, s2) == s2);

    BOOST_TEST(val(3)() == 3);
    BOOST_TEST(var(i)() == 4);
    BOOST_TEST(var(++i)() == 5);

    return boost::report_errors();    
}
