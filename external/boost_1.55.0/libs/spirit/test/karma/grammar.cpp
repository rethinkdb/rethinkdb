//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <string>
#include <iostream>

#include "test.hpp"

using namespace spirit_test;
using namespace boost::spirit;
using namespace boost::spirit::ascii;

typedef spirit_test::output_iterator<char>::type outiter_type;

struct num_list : karma::grammar<outiter_type, space_type>
{
    num_list() : num_list::base_type(start)
    {
        using boost::spirit::int_;
        num1 = int_(123);
        num2 = int_(456);
        num3 = int_(789);
        start = num1 << ',' << num2 << ',' << num3;
    }

    karma::rule<outiter_type, space_type> start, num1, num2, num3;
};

int
main()
{
    { // simple grammar test
        num_list nlist;
        BOOST_TEST(test_delimited("123 , 456 , 789 ", nlist, space));
    }

    { // direct access to the rules
        num_list def;
        BOOST_TEST(test_delimited("123 ", def.num1, space));
        BOOST_TEST(test_delimited("123 , 456 , 789 ", def.start, space));
    }

    return boost::report_errors();
}

