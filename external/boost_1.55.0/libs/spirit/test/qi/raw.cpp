/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>

#include <iostream>
#include <string>
#include "test.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using namespace boost::spirit::ascii;
    using boost::spirit::qi::raw;
    using boost::spirit::qi::eps;

    {
        boost::iterator_range<char const*> range;
        std::string str;
        BOOST_TEST((test_attr("spirit_test_123", raw[alpha >> *(alnum | '_')], range)));
        BOOST_TEST((std::string(range.begin(), range.end()) == "spirit_test_123"));
        BOOST_TEST((test_attr("  spirit", raw[*alpha], range, space)));
        BOOST_TEST((range.size() == 6));
    }

    {
        std::string str;
        BOOST_TEST((test_attr("spirit_test_123", raw[alpha >> *(alnum | '_')], str)));
        BOOST_TEST((str == "spirit_test_123"));
    }

    {
        boost::iterator_range<char const*> range;
        BOOST_TEST((test("x", raw[alpha])));
        BOOST_TEST((test_attr("x", raw[alpha], range)));
        //~ BOOST_TEST((test_attr("x", raw[alpha] >> eps, range)));
    }

    return boost::report_errors();
}
