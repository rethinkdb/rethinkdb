/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

namespace phoenix = boost::phoenix;

int
main()
{
    using phoenix::val;
    using phoenix::ref;
    using phoenix::arg_names::arg1;
    {
        BOOST_TEST((-val(123))() == -123);
        BOOST_TEST((- -val(123))() == 123);
        BOOST_TEST((+val(123))() == 123);
    }

    {
        int x = 123;

        BOOST_TEST((++ref(x))() == 124);
        BOOST_TEST(x == 124);
        BOOST_TEST((--ref(x))() == 123);
        BOOST_TEST(x == 123);

        BOOST_TEST((ref(x)++)() == 123);
        BOOST_TEST(x == 124);
        BOOST_TEST((ref(x)--)() == 124);
        BOOST_TEST(x == 123);

        int& r1 = (++ref(x))(); // should be an lvalue
        int& r2 = (--ref(x))(); // should be an lvalue
        BOOST_TEST(r1 == 123 && r2 == 123);
    }

    {   // From Phoenix 1.1 unary tests

        int i1 = 1, i = 5;

        BOOST_TEST((!val(true))() == false);
        BOOST_TEST((-val(1))() == -1);
        BOOST_TEST((+val(1))() == +1);
        BOOST_TEST((~val(1))() == ~1);
        BOOST_TEST(*(&arg1)(i1) == *(&i1));
        BOOST_TEST((&arg1)(i1) == &i1);

        BOOST_TEST((*val(&i1))() == *(&i1));
        BOOST_TEST((*&arg1)(i1) == *(&i1));
        BOOST_TEST((++ref(i))() == 6);
        BOOST_TEST((--ref(i))() == 5);
        BOOST_TEST((ref(i)++)() == 5);
        BOOST_TEST(i == 6);
        BOOST_TEST((ref(i)--)() == 6);
        BOOST_TEST(i == 5);
    }

    return boost::report_errors();
}
