/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifdef __GNUC__   //  Darn these relops!
#ifndef __SGI_STL_INTERNAL_RELOPS
#define __SGI_STL_INTERNAL_RELOPS
#endif
#endif

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
    int i2 = 2, i3 = 3, i = 5;
    const char* world = " world";

///////////////////////////////////////////////////////////////////////////////
//
//  Binary operators
//
///////////////////////////////////////////////////////////////////////////////
    BOOST_TEST((var(i) = var(i))() == 5);
    BOOST_TEST((var(i) = 3)() == 3);
    BOOST_TEST(i == 3);
    i = 5;
    int x, y, z;
    (var(x) = var(y) = var(z) = 10)();
    BOOST_TEST(x == 10 && y == 10 && z == 10);
    BOOST_TEST((val(world)[3])() == world[3]);

    BOOST_TEST((var(i) += 5)() == 10);
    BOOST_TEST((var(i) -= 5)() == 5);
    BOOST_TEST((var(i) *= 5)() == 25);
    BOOST_TEST((var(i) /= 5)() == 5);
    BOOST_TEST((var(i) %= 2)() == 1);

    BOOST_TEST((var(i) <<= 3)() == 8);
    BOOST_TEST((var(i) >>= 1)() == 4);
    BOOST_TEST((var(i) |= 0xFF)() == 0xFF);
    BOOST_TEST((var(i) &= 0xF0)() == 0xF0);
    BOOST_TEST((var(i) ^= 0xFFFFFFFF)() == int(0xFFFFFF0F));

    BOOST_TEST((val(5) == val(5))());
    BOOST_TEST((val(5) == 5)());

    BOOST_TEST((arg1 + arg2)(i2, i3) == i2 + i3);
    BOOST_TEST((arg1 - arg2)(i2, i3) == i2 - i3);
    BOOST_TEST((arg1 * arg2)(i2, i3) == i2 * i3);
    BOOST_TEST((arg1 / arg2)(i2, i3) == i2 / i3);
    BOOST_TEST((arg1 % arg2)(i2, i3) == i2 % i3);
    BOOST_TEST((arg1 & arg2)(i2, i3) == (i2 & i3));
    BOOST_TEST((arg1 | arg2)(i2, i3) == (i2 | i3));
    BOOST_TEST((arg1 ^ arg2)(i2, i3) == (i2 ^ i3));
    BOOST_TEST((arg1 << arg2)(i2, i3) == i2 << i3);
    BOOST_TEST((arg1 >> arg2)(i2, i3) == i2 >> i3);

    BOOST_TEST((val(5) != val(6))());
    BOOST_TEST((val(5) < val(6))());
    BOOST_TEST(!(val(5) > val(6))());
    BOOST_TEST((val(5) < val(6))());
    BOOST_TEST((val(5) <= val(6))());
    BOOST_TEST((val(5) <= val(5))());
    BOOST_TEST((val(7) >= val(6))());
    BOOST_TEST((val(7) >= val(7))());

    BOOST_TEST((val(false) && val(false))() == false);
    BOOST_TEST((val(true) && val(false))() == false);
    BOOST_TEST((val(false) && val(true))() == false);
    BOOST_TEST((val(true) && val(true))() == true);

    BOOST_TEST((val(false) || val(false))() == false);
    BOOST_TEST((val(true) || val(false))() == true);
    BOOST_TEST((val(false) || val(true))() == true);
    BOOST_TEST((val(true) || val(true))() == true);

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();
}
