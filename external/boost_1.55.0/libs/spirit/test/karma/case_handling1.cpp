//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this file intentionally contains non-ascii characters
// boostinspect:noascii

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_directive.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("x", lower['X']));
        BOOST_TEST(test("x", lower['x']));

        BOOST_TEST(test("x", lower[char_], 'X'));
        BOOST_TEST(test("x", lower[char_], 'x'));
        BOOST_TEST(test("x", lower[char_('X')]));
        BOOST_TEST(test("x", lower[char_('x')]));

        BOOST_TEST(test(" ", lower[space]));
        BOOST_TEST(test("\t", lower[space], '\t'));

        BOOST_TEST(test("x", lower[lower['X']]));
        BOOST_TEST(test("x", lower[lower['x']]));

        BOOST_TEST(test("x", lower[lower[char_]], 'X'));
        BOOST_TEST(test("x", lower[lower[char_]], 'x'));
        BOOST_TEST(test("x", lower[lower[char_('X')]]));
        BOOST_TEST(test("x", lower[lower[char_('x')]]));

        BOOST_TEST(test(" ", lower[lower[space]]));
        BOOST_TEST(test("\t", lower[lower[space]], '\t'));

        BOOST_TEST(test("X", upper[lower['X']]));
        BOOST_TEST(test("X", upper[lower['x']]));

        BOOST_TEST(test("X", upper[lower[char_]], 'X'));
        BOOST_TEST(test("X", upper[lower[char_]], 'x'));
        BOOST_TEST(test("X", upper[lower[char_('X')]]));
        BOOST_TEST(test("X", upper[lower[char_('x')]]));

        BOOST_TEST(test(" ", upper[lower[space]]));
        BOOST_TEST(test("\t", upper[lower[space]], '\t'));

        BOOST_TEST(test("X", upper['X']));
        BOOST_TEST(test("X", upper['x']));

        BOOST_TEST(test("X", upper[char_], 'X'));
        BOOST_TEST(test("X", upper[char_], 'x'));
        BOOST_TEST(test("X", upper[char_('X')]));
        BOOST_TEST(test("X", upper[char_('x')]));

        BOOST_TEST(test(" ", upper[space]));
        BOOST_TEST(test("\t", upper[space], '\t'));

        BOOST_TEST(test("x", lower[upper['X']]));
        BOOST_TEST(test("x", lower[upper['x']]));

        BOOST_TEST(test("x", lower[upper[char_]], 'X'));
        BOOST_TEST(test("x", lower[upper[char_]], 'x'));
        BOOST_TEST(test("x", lower[upper[char_('X')]]));
        BOOST_TEST(test("x", lower[upper[char_('x')]]));

        BOOST_TEST(test(" ", lower[upper[space]]));
        BOOST_TEST(test("\t", lower[upper[space]], '\t'));

        BOOST_TEST(test("X", upper[upper['X']]));
        BOOST_TEST(test("X", upper[upper['x']]));

        BOOST_TEST(test("X", upper[upper[char_]], 'X'));
        BOOST_TEST(test("X", upper[upper[char_]], 'x'));
        BOOST_TEST(test("X", upper[upper[char_('X')]]));
        BOOST_TEST(test("X", upper[upper[char_('x')]]));

        BOOST_TEST(test(" ", upper[upper[space]]));
        BOOST_TEST(test("\t", upper[upper[space]], '\t'));
    }

    return boost::report_errors();
}
