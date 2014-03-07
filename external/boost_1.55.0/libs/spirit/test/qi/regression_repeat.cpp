//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2010 Head Geek
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream> 
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi.hpp> 
 
namespace qi = boost::spirit::qi; 
using qi::omit; 
using qi::repeat; 
using std::cout; 
using std::endl; 
 
typedef qi::rule<std::string::const_iterator, std::string()> strrule_type; 
 
void test(const std::string input, strrule_type rule, std::string result) 
{ 
    std::string target; 
    std::string::const_iterator i = input.begin(), ie = input.end(); 
 
    BOOST_TEST(qi::parse(i, ie, rule, target) && target == result);
} 
 
int main() 
{ 
    strrule_type obsolete_year = 
        omit[-qi::char_(" \t")] >> 
        repeat(2)[qi::digit] >> 
        omit[-qi::char_(" \t")]; 
    strrule_type correct_year = repeat(4)[qi::digit]; 
 
    test("1776", qi::hold[correct_year] | repeat(2)[qi::digit], "1776");
    test("76",   obsolete_year, "76");
    test("76",   qi::hold[obsolete_year] | correct_year, "76");
    test(" 76",  qi::hold[correct_year] | obsolete_year, "76");
    test("76",   qi::hold[correct_year] | obsolete_year, "76");
    test("76",   qi::hold[correct_year] | repeat(2)[qi::digit], "76");

    return boost::report_errors();
} 

