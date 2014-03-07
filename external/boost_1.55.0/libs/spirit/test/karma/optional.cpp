//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include "test.hpp"

int main()
{
    using namespace spirit_test;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    {
        boost::optional<int> opt;
        BOOST_TEST(test("", -int_, opt));

        opt = 10;
        BOOST_TEST(test("10", -int_, opt));
    }

    {
        int opt = 10;
        BOOST_TEST(test("10", -int_, opt));
    }

    {
        boost::optional<int> opt;
        BOOST_TEST(test_delimited("", -int_, opt, space));

        opt = 10;
        BOOST_TEST(test_delimited("10 ", -int_, opt, space));
    }

    {
        int opt = 10;
        BOOST_TEST(test_delimited("10 ", -int_, opt, space));
    }

    {   // test action
        using namespace boost::phoenix;
        namespace phoenix = boost::phoenix;

        boost::optional<int> n ;
        BOOST_TEST(test("", (-int_)[_1 = phoenix::ref(n)]));

        n = 1234;
        BOOST_TEST(test("1234", (-int_)[_1 = phoenix::ref(n)]));
    }

    {   // test action
        using namespace boost::phoenix;
        namespace phoenix = boost::phoenix;

        int n = 1234;
        BOOST_TEST(test("1234", (-int_)[_1 = phoenix::ref(n)]));
    }

    {   // test action
        using namespace boost::phoenix;
        namespace phoenix = boost::phoenix;

        boost::optional<int> n;
        BOOST_TEST(test_delimited("", (-int_)[_1 = phoenix::ref(n)], space));

        n = 1234;
        BOOST_TEST(test_delimited("1234 ", (-int_)[_1 = phoenix::ref(n)], space));
    }

    {   // test action
        using namespace boost::phoenix;
        namespace phoenix = boost::phoenix;

        int n = 1234;
        BOOST_TEST(test_delimited("1234 ", (-int_)[_1 = phoenix::ref(n)], space));
    }

    return boost::report_errors();
}
