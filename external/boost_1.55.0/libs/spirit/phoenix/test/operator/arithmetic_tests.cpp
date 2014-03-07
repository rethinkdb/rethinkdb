/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

using namespace boost::phoenix;
using namespace std;

int
main()
{
    {
        int x = 123;

        BOOST_TEST((ref(x) += 456)() == 123 + 456);
        BOOST_TEST(x == 123 + 456);
        BOOST_TEST((ref(x) -= 456)() == 123);
        BOOST_TEST(x == 123);
        BOOST_TEST((ref(x) *= 456)() == 123 * 456);
        BOOST_TEST(x == 123 * 456);
        BOOST_TEST((ref(x) /= 456)() == 123);
        BOOST_TEST(x == 123);

        int& r1 = (ref(x) += 456)(); // should be an lvalue
        int& r2 = (ref(x) -= 456)(); // should be an lvalue
        int& r3 = (ref(x) *= 456)(); // should be an lvalue
        int& r4 = (ref(x) /= 456)(); // should be an lvalue
        BOOST_TEST(r1 == 123 && r2 == 123 && r3 == 123 && r4 == 123);

        BOOST_TEST((ref(x) %= 456)() == 123 % 456);
        BOOST_TEST(x == 123 % 456);
    }

    {
        BOOST_TEST((val(123) + 456)() == 123 + 456);
        BOOST_TEST((val(123) - 456)() == 123 - 456);
        BOOST_TEST((val(123) * 456)() == 123 * 456);
        BOOST_TEST((val(123) / 456)() == 123 / 456);
        BOOST_TEST((val(123) % 456)() == 123 % 456);

        BOOST_TEST((123 + val(456))() == 123 + 456);
        BOOST_TEST((123 - val(456))() == 123 - 456);
        BOOST_TEST((123 * val(456))() == 123 * 456);
        BOOST_TEST((123 / val(456))() == 123 / 456);
        BOOST_TEST((123 % val(456))() == 123 % 456);
    }

    return boost::report_errors();
}
