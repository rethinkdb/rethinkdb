/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    namespace qi = boost::spirit::qi;

    using qi::attr;
    using qi::double_;

    {
        double d = 0.0;
        BOOST_TEST(test_attr("", attr(1.0), d) && d == 1.0);

        double d1 = 1.0;
        BOOST_TEST(test_attr("", attr(d1), d) && d == 1.0);

        std::pair<double, double> p;
        BOOST_TEST(test_attr("1.0", double_ >> attr(1.0), p) && 
            p.first == 1.0 && p.second == 1.0);

        char c = '\0';
        BOOST_TEST(test_attr("", attr('a'), c) && c == 'a');
        std::string str;
        BOOST_TEST(test_attr("", attr("test"), str) && str == "test");
    }

    {   // testing lazy constructs
        using boost::phoenix::val;
        using boost::phoenix::ref;

        double d = 0.0;
        BOOST_TEST(test_attr("", attr(val(1.0)), d) && d == 1.0);

        double d1 = 2.0;
        BOOST_TEST(test_attr("", attr(ref(d1)), d) && d == 2.0);
    }

    {
        std::string s;
        BOOST_TEST(test_attr("s", "s" >> qi::attr(std::string("123")), s) && 
            s == "123");
    }

    return boost::report_errors();
}
