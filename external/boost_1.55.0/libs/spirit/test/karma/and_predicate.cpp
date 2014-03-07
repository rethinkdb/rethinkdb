//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_char.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace spirit_test;
    using namespace boost::spirit;

    {
        BOOST_TEST(test("1", int_(1) << &(int_(2) << &int_(3) << int_(4))));
    }

    {
        using boost::spirit::ascii::char_;

        BOOST_TEST(test("b", &char_('a') << 'b' | 'c', 'a'));
        BOOST_TEST(test("c", &char_('a') << 'b' | 'c', 'x'));
    }

    return boost::report_errors();
}
