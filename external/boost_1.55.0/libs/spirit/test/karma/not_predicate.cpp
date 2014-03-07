//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace spirit_test;
    using namespace boost::spirit;

    {
        using boost::spirit::ascii::char_;
        using boost::spirit::ascii::string;

        BOOST_TEST(test("c", !char_('a') << 'b' | 'c', 'a'));
        BOOST_TEST(test("b", !char_('a') << 'b' | 'c', 'x'));

        BOOST_TEST(test("def", !string("123") << "abc" | "def", "123"));
        BOOST_TEST(test("abc", !string("123") << "abc" | "def", "456"));
    }

    return boost::report_errors();
}
