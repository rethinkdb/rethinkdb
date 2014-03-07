/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_directive.hpp>

#include <iostream>
#include "test.hpp"

namespace qi = boost::spirit::qi;

int main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using qi::matches;
    using qi::char_;

    {
        BOOST_TEST(test("x", matches[char_]));
        bool result = false;
        BOOST_TEST(test_attr("x", matches[char_], result) && result);
    }

    {
        BOOST_TEST(!test("y", matches[char_('x')]));
        BOOST_TEST(!test("y", matches['x']));
        bool result = true;
        BOOST_TEST(test_attr("y", matches[char_('x')], result, false) && !result);
        result = true;
        BOOST_TEST(test_attr("y", matches['x'], result, false) && !result);
    }

    return boost::report_errors();
}
