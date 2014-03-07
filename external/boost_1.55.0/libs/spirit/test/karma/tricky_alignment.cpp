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
        BOOST_TEST(test("x         *****", left_align(15, '*')[left_align[char_('x')]]));
        BOOST_TEST(test("     x    *****", left_align(15, '*')[center[char_('x')]]));
        BOOST_TEST(test("         x*****", left_align(15, '*')[right_align[char_('x')]]));

        BOOST_TEST(test("*****x         ", right_align(15, '*')[left_align[char_('x')]]));
        BOOST_TEST(test("*****     x    ", right_align(15, '*')[center[char_('x')]]));
        BOOST_TEST(test("*****         x", right_align(15, '*')[right_align[char_('x')]]));

        BOOST_TEST(test("***x         **", center(15, '*')[left_align[char_('x')]]));
        BOOST_TEST(test("***     x    **", center(15, '*')[center[char_('x')]]));
        BOOST_TEST(test("***         x**", center(15, '*')[right_align[char_('x')]]));

        BOOST_TEST(test("ab        *****", left_align(15, '*')[left_align[lit("ab")]]));
        BOOST_TEST(test("    ab    *****", left_align(15, '*')[center[lit("ab")]]));
        BOOST_TEST(test("        ab*****", left_align(15, '*')[right_align[lit("ab")]]));

        BOOST_TEST(test("*****ab        ", right_align(15, '*')[left_align[lit("ab")]]));
        BOOST_TEST(test("*****    ab    ", right_align(15, '*')[center[lit("ab")]]));
        BOOST_TEST(test("*****        ab", right_align(15, '*')[right_align[lit("ab")]]));

        BOOST_TEST(test("***ab        **", center(15, '*')[left_align[lit("ab")]]));
        BOOST_TEST(test("***    ab    **", center(15, '*')[center[lit("ab")]]));
        BOOST_TEST(test("***        ab**", center(15, '*')[right_align[lit("ab")]]));
    }

    {
        BOOST_TEST(test("x         ******", left_align(16, '*')[left_align[char_('x')]]));
        BOOST_TEST(test("     x    ******", left_align(16, '*')[center[char_('x')]]));
        BOOST_TEST(test("         x******", left_align(16, '*')[right_align[char_('x')]]));

        BOOST_TEST(test("******x         ", right_align(16, '*')[left_align[char_('x')]]));
        BOOST_TEST(test("******     x    ", right_align(16, '*')[center[char_('x')]]));
        BOOST_TEST(test("******         x", right_align(16, '*')[right_align[char_('x')]]));

        BOOST_TEST(test("***x         ***", center(16, '*')[left_align[char_('x')]]));
        BOOST_TEST(test("***     x    ***", center(16, '*')[center[char_('x')]]));
        BOOST_TEST(test("***         x***", center(16, '*')[right_align[char_('x')]]));

        BOOST_TEST(test("ab        ******", left_align(16, '*')[left_align[lit("ab")]]));
        BOOST_TEST(test("    ab    ******", left_align(16, '*')[center[lit("ab")]]));
        BOOST_TEST(test("        ab******", left_align(16, '*')[right_align[lit("ab")]]));

        BOOST_TEST(test("******ab        ", right_align(16, '*')[left_align[lit("ab")]]));
        BOOST_TEST(test("******    ab    ", right_align(16, '*')[center[lit("ab")]]));
        BOOST_TEST(test("******        ab", right_align(16, '*')[right_align[lit("ab")]]));

        BOOST_TEST(test("***ab        ***", center(16, '*')[left_align[lit("ab")]]));
        BOOST_TEST(test("***    ab    ***", center(16, '*')[center[lit("ab")]]));
        BOOST_TEST(test("***        ab***", center(16, '*')[right_align[lit("ab")]]));
    }

    {
        BOOST_TEST(test("x          ****", left_align(15, '*')[left_align(11)[char_('x')]]));
        BOOST_TEST(test("     x     ****", left_align(15, '*')[center(11)[char_('x')]]));
        BOOST_TEST(test("          x****", left_align(15, '*')[right_align(11)[char_('x')]]));

        BOOST_TEST(test("****x          ", right_align(15, '*')[left_align(11)[char_('x')]]));
        BOOST_TEST(test("****     x     ", right_align(15, '*')[center(11)[char_('x')]]));
        BOOST_TEST(test("****          x", right_align(15, '*')[right_align(11)[char_('x')]]));

        BOOST_TEST(test("**x          **", center(15, '*')[left_align(11)[char_('x')]]));
        BOOST_TEST(test("**     x     **", center(15, '*')[center(11)[char_('x')]]));
        BOOST_TEST(test("**          x**", center(15, '*')[right_align(11)[char_('x')]]));

        BOOST_TEST(test("ab         ****", left_align(15, '*')[left_align(11)[lit("ab")]]));
        BOOST_TEST(test("     ab    ****", left_align(15, '*')[center(11)[lit("ab")]]));
        BOOST_TEST(test("         ab****", left_align(15, '*')[right_align(11)[lit("ab")]]));

        BOOST_TEST(test("****ab         ", right_align(15, '*')[left_align(11)[lit("ab")]]));
        BOOST_TEST(test("****     ab    ", right_align(15, '*')[center(11)[lit("ab")]]));
        BOOST_TEST(test("****         ab", right_align(15, '*')[right_align(11)[lit("ab")]]));

        BOOST_TEST(test("**ab         **", center(15, '*')[left_align(11)[lit("ab")]]));
        BOOST_TEST(test("**     ab    **", center(15, '*')[center(11)[lit("ab")]]));
        BOOST_TEST(test("**         ab**", center(15, '*')[right_align(11)[lit("ab")]]));
    }

    {
        BOOST_TEST(test("x          *****", left_align(16, '*')[left_align(11)[char_('x')]]));
        BOOST_TEST(test("     x     *****", left_align(16, '*')[center(11)[char_('x')]]));
        BOOST_TEST(test("          x*****", left_align(16, '*')[right_align(11)[char_('x')]]));

        BOOST_TEST(test("*****x          ", right_align(16, '*')[left_align(11)[char_('x')]]));
        BOOST_TEST(test("*****     x     ", right_align(16, '*')[center(11)[char_('x')]]));
        BOOST_TEST(test("*****          x", right_align(16, '*')[right_align(11)[char_('x')]]));

        BOOST_TEST(test("***x          **", center(16, '*')[left_align(11)[char_('x')]]));
        BOOST_TEST(test("***     x     **", center(16, '*')[center(11)[char_('x')]]));
        BOOST_TEST(test("***          x**", center(16, '*')[right_align(11)[char_('x')]]));

        BOOST_TEST(test("ab         *****", left_align(16, '*')[left_align(11)[lit("ab")]]));
        BOOST_TEST(test("     ab    *****", left_align(16, '*')[center(11)[lit("ab")]]));
        BOOST_TEST(test("         ab*****", left_align(16, '*')[right_align(11)[lit("ab")]]));

        BOOST_TEST(test("*****ab         ", right_align(16, '*')[left_align(11)[lit("ab")]]));
        BOOST_TEST(test("*****     ab    ", right_align(16, '*')[center(11)[lit("ab")]]));
        BOOST_TEST(test("*****         ab", right_align(16, '*')[right_align(11)[lit("ab")]]));

        BOOST_TEST(test("***ab         **", center(16, '*')[left_align(11)[lit("ab")]]));
        BOOST_TEST(test("***     ab    **", center(16, '*')[center(11)[lit("ab")]]));
        BOOST_TEST(test("***         ab**", center(16, '*')[right_align(11)[lit("ab")]]));
    }

    return boost::report_errors();
}
