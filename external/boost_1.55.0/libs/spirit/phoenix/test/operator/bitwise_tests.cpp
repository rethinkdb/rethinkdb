/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;

int
main()
{
    {
        int x;
        int y;

        x = 123;
        y = 123;
        (ref(x) &= 456)();
        y &= 456;
        BOOST_TEST(x == y);

        x = 123;
        y = 123;
        (ref(x) |= 456)();
        y |= 456;
        BOOST_TEST(x == y);

        x = 123;
        y = 123;
        (ref(x) ^= 456)();
        y ^= 456;
        BOOST_TEST(x == y);

        x = 123;
        y = 123;
        (ref(x) <<= 4)();
        y <<= 4;
        BOOST_TEST(x == y);

        x = 1230000;
        y = 1230000;
        (ref(x) >>= 4)();
        y >>= 4;
        BOOST_TEST(x == y);

        int& r1 = (ref(x) &= 456)(); // should be an lvalue
        int& r2 = (ref(x) |= 456)(); // should be an lvalue
        int& r3 = (ref(x) ^= 456)(); // should be an lvalue
        int& r4 = (ref(x) <<= 4)(); // should be an lvalue
        int& r5 = (ref(x) >>= 4)(); // should be an lvalue
        BOOST_TEST(&r1 == &r2 && &r2 == &r3 && &r3 == &r4 && &r4 == &r5);
    }

    {
        BOOST_TEST((val(123) & 456)() == (123 & 456));
        BOOST_TEST((val(123) | 456)() == (123 | 456));
        BOOST_TEST((val(123) ^ 456)() == (123 ^ 456));
        BOOST_TEST((val(123) << 4)() == (123 << 4));
        BOOST_TEST((val(1230000) >> 4)() == (1230000 >> 4));

        char const* s = "Yabadabadoo!!!\n";
        (cout << arg1)(s);
    }

    return boost::report_errors();
}
