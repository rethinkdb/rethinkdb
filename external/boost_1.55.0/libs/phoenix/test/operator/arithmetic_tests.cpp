/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

#include <boost/bind.hpp>

namespace phoenix = boost::phoenix;

int
main()
{
    using phoenix::ref;
    using phoenix::val;

    {
        int x = 123;

        BOOST_TEST((ref(x) += 456)() == (123 + 456));
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
        BOOST_TEST((-val(123))() == -123);
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
    {
        // Testcase contributed from Philipp Reh, failed in Phoenix V2
        using boost::phoenix::arg_names::arg1;
        using boost::phoenix::arg_names::arg2;

        int x[2];

        BOOST_TEST((&arg1 - &arg2)(x[0], x[1]) == &x[0] - &x[1]);
        BOOST_TEST((arg1 - arg2)(&x[0], &x[1]) == &x[0] - &x[1]);
    }

    return boost::report_errors();
}
