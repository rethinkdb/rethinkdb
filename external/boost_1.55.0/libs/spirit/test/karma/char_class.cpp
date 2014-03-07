//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this file intentionally contains non-ascii characters
// boostinspect:noascii

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_directive.hpp>

#include <string>
#include <iterator>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace spirit_test;
    using namespace boost::spirit;

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("a", alpha, 'a'));
        BOOST_TEST(!test("", alpha, '1'));
        BOOST_TEST(test(" ", blank, ' '));
        BOOST_TEST(!test("", blank, 'x'));
        BOOST_TEST(test("1", digit, '1'));
        BOOST_TEST(!test("", digit, 'x'));
        BOOST_TEST(test("a", lower, 'a'));
        BOOST_TEST(!test("", lower, 'A'));
        BOOST_TEST(test("!", punct, '!'));
        BOOST_TEST(!test("", punct, 'x'));
        BOOST_TEST(test(" ", space));
        BOOST_TEST(test(" ", space, ' '));
        BOOST_TEST(!test("", space, '\n'));
        BOOST_TEST(test("\r", space, '\r'));
        BOOST_TEST(test("\t", space, '\t'));
        BOOST_TEST(test("A", upper, 'A'));
        BOOST_TEST(!test("", upper, 'a'));
        BOOST_TEST(test("A", xdigit, 'A'));
        BOOST_TEST(test("0", xdigit, '0'));
        BOOST_TEST(test("f", xdigit, 'f'));
        BOOST_TEST(!test("", xdigit, 'g'));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(!test("", ~alpha, 'a'));
        BOOST_TEST(test("1", ~alpha, '1'));
        BOOST_TEST(!test("", ~blank, ' '));
        BOOST_TEST(test("x", ~blank, 'x'));
        BOOST_TEST(!test("", ~digit, '1'));
        BOOST_TEST(test("x", ~digit, 'x'));
        BOOST_TEST(!test("", ~lower, 'a'));
        BOOST_TEST(test("A", ~lower, 'A'));
        BOOST_TEST(!test("", ~punct, '!'));
        BOOST_TEST(test("x", ~punct, 'x'));
        BOOST_TEST(!test("", ~space));
        BOOST_TEST(!test("", ~space, ' '));
        BOOST_TEST(!test("", ~space, '\r'));
        BOOST_TEST(!test("", ~space, '\t'));
        BOOST_TEST(!test("", ~upper, 'A'));
        BOOST_TEST(test("a", ~upper, 'a'));
        BOOST_TEST(!test("", ~xdigit, 'A'));
        BOOST_TEST(!test("", ~xdigit, '0'));
        BOOST_TEST(!test("", ~xdigit, 'f'));
        BOOST_TEST(test("g", ~xdigit, 'g'));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("a", ~~alpha, 'a'));
        BOOST_TEST(!test("", ~~alpha, '1'));
        BOOST_TEST(test(" ", ~~blank, ' '));
        BOOST_TEST(!test("", ~~blank, 'x'));
        BOOST_TEST(test("1", ~~digit, '1'));
        BOOST_TEST(!test("", ~~digit, 'x'));
        BOOST_TEST(test("a", ~~lower, 'a'));
        BOOST_TEST(!test("", ~~lower, 'A'));
        BOOST_TEST(test("!", ~~punct, '!'));
        BOOST_TEST(!test("", ~~punct, 'x'));
        BOOST_TEST(test(" ", ~~space));
        BOOST_TEST(test(" ", ~~space, ' '));
        BOOST_TEST(!test("", ~~space, '\n'));
        BOOST_TEST(test("\r", ~~space, '\r'));
        BOOST_TEST(test("\t", ~~space, '\t'));
        BOOST_TEST(test("A", ~~upper, 'A'));
        BOOST_TEST(!test("", ~~upper, 'a'));
        BOOST_TEST(test("A", ~~xdigit, 'A'));
        BOOST_TEST(test("0", ~~xdigit, '0'));
        BOOST_TEST(test("f", ~~xdigit, 'f'));
        BOOST_TEST(!test("", ~~xdigit, 'g'));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("a", lower[alpha], 'a'));
        BOOST_TEST(!test("", lower[alpha], 'A'));
        BOOST_TEST(!test("", lower[alpha], '1'));
        BOOST_TEST(test("a", lower[alnum], 'a'));
        BOOST_TEST(!test("", lower[alnum], 'A'));
        BOOST_TEST(test("1", lower[alnum], '1'));

        BOOST_TEST(!test("", upper[alpha], 'a'));
        BOOST_TEST(test("A", upper[alpha], 'A'));
        BOOST_TEST(!test("", upper[alpha], '1'));
        BOOST_TEST(!test("", upper[alnum], 'a'));
        BOOST_TEST(test("A", upper[alnum], 'A'));
        BOOST_TEST(test("1", upper[alnum], '1'));
    }

    {
        using namespace boost::spirit::iso8859_1;
        BOOST_TEST(test("a", alpha, 'a'));
        BOOST_TEST(!test("", alpha, '1'));
        BOOST_TEST(test(" ", blank, ' '));
        BOOST_TEST(!test("", blank, 'x'));
        BOOST_TEST(test("1", digit, '1'));
        BOOST_TEST(!test("", digit, 'x'));
        BOOST_TEST(test("a", lower, 'a'));
        BOOST_TEST(!test("", lower, 'A'));
        BOOST_TEST(test("!", punct, '!'));
        BOOST_TEST(!test("", punct, 'x'));
        BOOST_TEST(test(" ", space));
        BOOST_TEST(test(" ", space, ' '));
        BOOST_TEST(!test("", space, '\n'));
        BOOST_TEST(test("\r", space, '\r'));
        BOOST_TEST(test("\t", space, '\t'));
        BOOST_TEST(test("A", upper, 'A'));
        BOOST_TEST(!test("", upper, 'a'));
        BOOST_TEST(test("A", xdigit, 'A'));
        BOOST_TEST(test("0", xdigit, '0'));
        BOOST_TEST(test("f", xdigit, 'f'));
        BOOST_TEST(!test("", xdigit, 'g'));


// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("german")
#endif
        BOOST_TEST(test("é", alpha, 'é'));
        BOOST_TEST(test("é", lower, 'é'));
        BOOST_TEST(!test("", upper, 'é'));
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif
    }

    {
        using namespace boost::spirit::standard;
        BOOST_TEST(test("a", alpha, 'a'));
        BOOST_TEST(!test("", alpha, '1'));
        BOOST_TEST(test(" ", blank, ' '));
        BOOST_TEST(!test("", blank, 'x'));
        BOOST_TEST(test("1", digit, '1'));
        BOOST_TEST(!test("", digit, 'x'));
        BOOST_TEST(test("a", lower, 'a'));
        BOOST_TEST(!test("", lower, 'A'));
        BOOST_TEST(test("!", punct, '!'));
        BOOST_TEST(!test("", punct, 'x'));
        BOOST_TEST(test(" ", space));
        BOOST_TEST(test(" ", space, ' '));
        BOOST_TEST(!test("", space, '\n'));
        BOOST_TEST(test("\r", space, '\r'));
        BOOST_TEST(test("\t", space, '\t'));
        BOOST_TEST(test("A", upper, 'A'));
        BOOST_TEST(!test("", upper, 'a'));
        BOOST_TEST(test("A", xdigit, 'A'));
        BOOST_TEST(test("0", xdigit, '0'));
        BOOST_TEST(test("f", xdigit, 'f'));
        BOOST_TEST(!test("", xdigit, 'g'));
    }

    {
        using namespace boost::spirit::standard_wide;
        BOOST_TEST(test(L"a", alpha, L'a'));
        BOOST_TEST(!test(L"", alpha, L'1'));
        BOOST_TEST(test(L" ", blank, L' '));
        BOOST_TEST(!test(L"", blank, L'x'));
        BOOST_TEST(test(L"1", digit, L'1'));
        BOOST_TEST(!test(L"", digit, L'x'));
        BOOST_TEST(test(L"a", lower, L'a'));
        BOOST_TEST(!test(L"", lower, L'A'));
        BOOST_TEST(test(L"!", punct, L'!'));
        BOOST_TEST(!test(L"", punct, L'x'));
        BOOST_TEST(test(L" ", space));
        BOOST_TEST(test(L" ", space, L' '));
        BOOST_TEST(!test(L"", space, L'\n'));
        BOOST_TEST(test(L"\r", space, L'\r'));
        BOOST_TEST(test(L"\t", space, L'\t'));
        BOOST_TEST(test(L"A", upper, L'A'));
        BOOST_TEST(!test(L"", upper, L'a'));
        BOOST_TEST(test(L"A", xdigit, L'A'));
        BOOST_TEST(test(L"0", xdigit, L'0'));
        BOOST_TEST(test(L"f", xdigit, L'f'));
        BOOST_TEST(!test(L"", xdigit, L'g'));
    }

    return boost::report_errors();
}
