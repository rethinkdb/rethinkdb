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
#include <boost/spirit/include/phoenix1_operators.hpp>

using namespace phoenix;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    int i1 = 1, i = 5;

///////////////////////////////////////////////////////////////////////////////
//
//  Unary operators
//
///////////////////////////////////////////////////////////////////////////////
    BOOST_TEST((!val(true))() == false);
    BOOST_TEST((-val(1))() == -1);
    BOOST_TEST((+val(1))() == +1);
    BOOST_TEST((~val(1))() == ~1);
    BOOST_TEST(*(&arg1)(i1) == *(&i1));
    BOOST_TEST((&arg1)(i1) == &i1);

    BOOST_TEST((*val(&i1))() == *(&i1));
    BOOST_TEST((*&arg1)(i1) == *(&i1));
    BOOST_TEST((++var(i))() == 6);
    BOOST_TEST((--var(i))() == 5);
    BOOST_TEST((var(i)++)() == 5);
    BOOST_TEST(i == 6);
    BOOST_TEST((var(i)--)() == 6);
    BOOST_TEST(i == 5);

    return boost::report_errors();    
}
