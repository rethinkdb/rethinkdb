//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_directive.hpp>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
int 
main()
{
    using namespace spirit_test;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    {
        BOOST_TEST(test("         x", right_align[char_('x')]));
        BOOST_TEST(test("         x", right_align[char_], 'x'));
        BOOST_TEST(test("         x", right_align['x']));
        
        BOOST_TEST(test("         x", right_align(10)[char_('x')]));
        BOOST_TEST(test("         x", right_align(10)[char_], 'x'));
        BOOST_TEST(test("         x", right_align(10)['x']));

        BOOST_TEST(test("*********x", right_align(10, char_('*'))[char_('x')]));
        BOOST_TEST(test("*********x", right_align(10, '*')[char_], 'x'));
        BOOST_TEST(test("*********x", right_align(10, '*')['x']));

        BOOST_TEST(test("*********x", right_align(char_('*'))[char_('x')]));
        BOOST_TEST(test("*********x", right_align(char_('*'))[char_], 'x'));
        BOOST_TEST(test("*********x", right_align(char_('*'))['x']));

        BOOST_TEST(test("       abc", right_align[lit("abc")]));
        BOOST_TEST(test("       abc", right_align[string], "abc"));
        
        BOOST_TEST(test("       abc", right_align(10)[lit("abc")]));
        BOOST_TEST(test("       abc", right_align(10)[string], "abc"));
        BOOST_TEST(test("       abc", right_align(10)["abc"]));

        BOOST_TEST(test("*******abc", right_align(10, char_('*'))[lit("abc")]));
        BOOST_TEST(test("*******abc", right_align(10, '*')[string], "abc"));
        BOOST_TEST(test("*******abc", right_align(10, '*')["abc"]));

        BOOST_TEST(test("*******abc", right_align(char_('*'))[lit("abc")]));
        BOOST_TEST(test("*******abc", right_align(char_('*'))[string], "abc"));
        BOOST_TEST(test("*******abc", right_align(char_('*'))["abc"]));

        BOOST_TEST(test("       100", right_align[int_(100)]));
        BOOST_TEST(test("       100", right_align[int_], 100));
                                
        BOOST_TEST(test("       100", right_align(10)[int_(100)]));
        BOOST_TEST(test("       100", right_align(10)[int_], 100));
                                
        BOOST_TEST(test("*******100", right_align(10, char_('*'))[int_(100)]));
        BOOST_TEST(test("*******100", right_align(10, '*')[int_], 100));
                                
        BOOST_TEST(test("*******100", right_align(char_('*'))[int_(100)]));
        BOOST_TEST(test("*******100", right_align(char_('*'))[int_], 100));
    }
    
    return boost::report_errors();
}
