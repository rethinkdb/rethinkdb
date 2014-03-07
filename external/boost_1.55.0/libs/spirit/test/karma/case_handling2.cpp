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
        using namespace boost::spirit::standard_wide;

        BOOST_TEST(test(L"x", lower[L'X']));
        BOOST_TEST(test(L"x", lower[L'x']));

        BOOST_TEST(test(L"x", lower[char_], L'X'));
        BOOST_TEST(test(L"x", lower[char_], L'x'));
        BOOST_TEST(test(L"x", lower[char_(L'X')]));
        BOOST_TEST(test(L"x", lower[char_(L'x')]));

        BOOST_TEST(test(L" ", lower[space]));
        BOOST_TEST(test(L"\t", lower[space], L'\t'));

        BOOST_TEST(test(L"x", lower[lower[L'X']]));
        BOOST_TEST(test(L"x", lower[lower[L'x']]));

        BOOST_TEST(test(L"x", lower[lower[char_]], L'X'));
        BOOST_TEST(test(L"x", lower[lower[char_]], L'x'));
        BOOST_TEST(test(L"x", lower[lower[char_(L'X')]]));
        BOOST_TEST(test(L"x", lower[lower[char_(L'x')]]));

        BOOST_TEST(test(L" ", lower[lower[space]]));
        BOOST_TEST(test(L"\t", lower[lower[space]], L'\t'));

        BOOST_TEST(test(L"X", upper[lower[L'X']]));
        BOOST_TEST(test(L"X", upper[lower[L'x']]));

        BOOST_TEST(test(L"X", upper[lower[char_]], L'X'));
        BOOST_TEST(test(L"X", upper[lower[char_]], L'x'));
        BOOST_TEST(test(L"X", upper[lower[char_(L'X')]]));
        BOOST_TEST(test(L"X", upper[lower[char_(L'x')]]));

        BOOST_TEST(test(L" ", upper[lower[space]]));
        BOOST_TEST(test(L"\t", upper[lower[space]], L'\t'));

        BOOST_TEST(test(L"X", upper[L'X']));
        BOOST_TEST(test(L"X", upper[L'x']));

        BOOST_TEST(test(L"X", upper[char_], L'X'));
        BOOST_TEST(test(L"X", upper[char_], L'x'));
        BOOST_TEST(test(L"X", upper[char_(L'X')]));
        BOOST_TEST(test(L"X", upper[char_(L'x')]));

        BOOST_TEST(test(L" ", upper[space]));
        BOOST_TEST(test(L"\t", upper[space], L'\t'));

        BOOST_TEST(test(L"x", lower[upper[L'X']]));
        BOOST_TEST(test(L"x", lower[upper[L'x']]));

        BOOST_TEST(test(L"x", lower[upper[char_]], L'X'));
        BOOST_TEST(test(L"x", lower[upper[char_]], L'x'));
        BOOST_TEST(test(L"x", lower[upper[char_(L'X')]]));
        BOOST_TEST(test(L"x", lower[upper[char_(L'x')]]));

        BOOST_TEST(test(L" ", lower[upper[space]]));
        BOOST_TEST(test(L"\t", lower[upper[space]], L'\t'));

        BOOST_TEST(test(L"X", upper[upper[L'X']]));
        BOOST_TEST(test(L"X", upper[upper[L'x']]));

        BOOST_TEST(test(L"X", upper[upper[char_]], L'X'));
        BOOST_TEST(test(L"X", upper[upper[char_]], L'x'));
        BOOST_TEST(test(L"X", upper[upper[char_(L'X')]]));
        BOOST_TEST(test(L"X", upper[upper[char_(L'x')]]));

        BOOST_TEST(test(L" ", upper[upper[space]]));
        BOOST_TEST(test(L"\t", upper[upper[space]], L'\t'));
    }

// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("german")
#endif
    {
        using namespace boost::spirit::iso8859_1;

        BOOST_TEST(test("ää", lower["Ää"]));
        BOOST_TEST(test("ää", lower["Ää"]));

        BOOST_TEST(test("ÄÄ", upper["Ää"]));
        BOOST_TEST(test("ÄÄ", upper["Ää"]));
    }
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    {
        using namespace boost::spirit::ascii;
        using boost::phoenix::val;

        BOOST_TEST(test("x", lower[val('X')]));
        BOOST_TEST(test("x", lower[val('x')]));
    }

    return boost::report_errors();
}
