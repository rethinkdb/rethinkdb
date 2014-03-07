/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// this file intentionally contains non-ascii characters
// boostinspect:noascii

#define BOOST_SPIRIT_UNICODE

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/support_attributes.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("1", alnum));
        BOOST_TEST(!test(" ", alnum));
        BOOST_TEST(!test("1", alpha));
        BOOST_TEST(test("x", alpha));
        BOOST_TEST(test(" ", blank));
        BOOST_TEST(!test("x", blank));
        BOOST_TEST(test("1", digit));
        BOOST_TEST(!test("x", digit));
        BOOST_TEST(test("a", lower));
        BOOST_TEST(!test("A", lower));
        BOOST_TEST(test("!", punct));
        BOOST_TEST(!test("x", punct));
        BOOST_TEST(test(" ", space));
        BOOST_TEST(test("\n", space));
        BOOST_TEST(test("\r", space));
        BOOST_TEST(test("\t", space));
        BOOST_TEST(test("A", upper));
        BOOST_TEST(!test("a", upper));
        BOOST_TEST(test("A", xdigit));
        BOOST_TEST(test("0", xdigit));
        BOOST_TEST(test("f", xdigit));
        BOOST_TEST(!test("g", xdigit));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(!test("1", ~alnum));
        BOOST_TEST(test(" ", ~alnum));
        BOOST_TEST(test("1", ~alpha));
        BOOST_TEST(!test("x", ~alpha));
        BOOST_TEST(!test(" ", ~blank));
        BOOST_TEST(test("x", ~blank));
        BOOST_TEST(!test("1", ~digit));
        BOOST_TEST(test("x", ~digit));
        BOOST_TEST(!test("a", ~lower));
        BOOST_TEST(test("A", ~lower));
        BOOST_TEST(!test("!", ~punct));
        BOOST_TEST(test("x", ~punct));
        BOOST_TEST(!test(" ", ~space));
        BOOST_TEST(!test("\n", ~space));
        BOOST_TEST(!test("\r", ~space));
        BOOST_TEST(!test("\t", ~space));
        BOOST_TEST(!test("A", ~upper));
        BOOST_TEST(test("a", ~upper));
        BOOST_TEST(!test("A", ~xdigit));
        BOOST_TEST(!test("0", ~xdigit));
        BOOST_TEST(!test("f", ~xdigit));
        BOOST_TEST(test("g", ~xdigit));
    }

    {
        // we use the hoisted qi namespace this time
        using namespace boost::spirit::qi::iso8859_1;
        BOOST_TEST(test("1", alnum));
        BOOST_TEST(!test(" ", alnum));
        BOOST_TEST(!test("1", alpha));
        BOOST_TEST(test("x", alpha));
        BOOST_TEST(test(" ", blank));
        BOOST_TEST(!test("x", blank));
        BOOST_TEST(test("1", digit));
        BOOST_TEST(!test("x", digit));
        BOOST_TEST(test("a", lower));
        BOOST_TEST(!test("A", lower));
        BOOST_TEST(test("!", punct));
        BOOST_TEST(!test("x", punct));
        BOOST_TEST(test(" ", space));
        BOOST_TEST(test("\n", space));
        BOOST_TEST(test("\r", space));
        BOOST_TEST(test("\t", space));
        BOOST_TEST(test("A", upper));
        BOOST_TEST(!test("a", upper));
        BOOST_TEST(test("A", xdigit));
        BOOST_TEST(test("0", xdigit));
        BOOST_TEST(test("f", xdigit));
        BOOST_TEST(!test("g", xdigit));

// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("german")
#endif
        BOOST_TEST(test("é", alpha));
        BOOST_TEST(test("é", lower));
        BOOST_TEST(!test("é", upper));
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif
    }

    {
        using namespace boost::spirit::standard;
        BOOST_TEST(test("1", alnum));
        BOOST_TEST(!test(" ", alnum));
        BOOST_TEST(!test("1", alpha));
        BOOST_TEST(test("x", alpha));
        BOOST_TEST(test(" ", blank));
        BOOST_TEST(!test("x", blank));
        BOOST_TEST(test("1", digit));
        BOOST_TEST(!test("x", digit));
        BOOST_TEST(test("a", lower));
        BOOST_TEST(!test("A", lower));
        BOOST_TEST(test("!", punct));
        BOOST_TEST(!test("x", punct));
        BOOST_TEST(test(" ", space));
        BOOST_TEST(test("\n", space));
        BOOST_TEST(test("\r", space));
        BOOST_TEST(test("\t", space));
        BOOST_TEST(test("A", upper));
        BOOST_TEST(!test("a", upper));
        BOOST_TEST(test("A", xdigit));
        BOOST_TEST(test("0", xdigit));
        BOOST_TEST(test("f", xdigit));
        BOOST_TEST(!test("g", xdigit));
    }

    {
        using namespace boost::spirit::standard_wide;
        BOOST_TEST(test(L"1", alnum));
        BOOST_TEST(!test(L" ", alnum));
        BOOST_TEST(!test(L"1", alpha));
        BOOST_TEST(test(L"x", alpha));
        BOOST_TEST(test(L" ", blank));
        BOOST_TEST(!test(L"x", blank));
        BOOST_TEST(test(L"1", digit));
        BOOST_TEST(!test(L"x", digit));
        BOOST_TEST(test(L"a", lower));
        BOOST_TEST(!test(L"A", lower));
        BOOST_TEST(test(L"!", punct));
        BOOST_TEST(!test(L"x", punct));
        BOOST_TEST(test(L" ", space));
        BOOST_TEST(test(L"\n", space));
        BOOST_TEST(test(L"\r", space));
        BOOST_TEST(test(L"\t", space));
        BOOST_TEST(test(L"A", upper));
        BOOST_TEST(!test(L"a", upper));
        BOOST_TEST(test(L"A", xdigit));
        BOOST_TEST(test(L"0", xdigit));
        BOOST_TEST(test(L"f", xdigit));
        BOOST_TEST(!test(L"g", xdigit));
    }

    {
        using namespace boost::spirit::unicode;
        BOOST_TEST(test(L"1", alnum));
        BOOST_TEST(!test(L" ", alnum));
        BOOST_TEST(!test(L"1", alpha));
        BOOST_TEST(test(L"x", alpha));
        BOOST_TEST(test(L" ", blank));
        BOOST_TEST(!test(L"x", blank));
        BOOST_TEST(test(L"1", digit));
        BOOST_TEST(!test(L"x", digit));
        BOOST_TEST(test(L"a", lower));
        BOOST_TEST(!test(L"A", lower));
        BOOST_TEST(test(L"!", punct));
        BOOST_TEST(!test(L"x", punct));
        BOOST_TEST(test(L" ", space));
        BOOST_TEST(test(L"\n", space));
        BOOST_TEST(test(L"\r", space));
        BOOST_TEST(test(L"\t", space));
        BOOST_TEST(test(L"A", upper));
        BOOST_TEST(!test(L"a", upper));
        BOOST_TEST(test(L"A", xdigit));
        BOOST_TEST(test(L"0", xdigit));
        BOOST_TEST(test(L"f", xdigit));
        BOOST_TEST(!test(L"g", xdigit));

// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("german")
#endif
        BOOST_TEST(test("é", alpha));
        BOOST_TEST(test("é", lower));
        BOOST_TEST(!test("é", upper));

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif
    }

    {   // test attribute extraction
        using boost::spirit::qi::domain;
        using boost::spirit::traits::attribute_of;
        using boost::spirit::iso8859_1::alpha;
        using boost::spirit::iso8859_1::alpha_type;
        using boost::spirit::result_of::compile;

        BOOST_STATIC_ASSERT((
            boost::is_same<
                attribute_of<compile<domain, alpha_type>::type>::type
              , unsigned char>::value));

        int attr = 0;
        BOOST_TEST(test_attr("a", alpha, attr));
        BOOST_TEST(attr == 'a');
    }

    {   // test attribute extraction
        using boost::spirit::iso8859_1::alpha;
        using boost::spirit::iso8859_1::space;
        char attr = 0;
        BOOST_TEST(test_attr("     a", alpha, attr, space));
        BOOST_TEST(attr == 'a');
    }

    {   // test action

        using namespace boost::spirit::ascii;
        using boost::phoenix::ref;
        using boost::spirit::_1;
        char ch;

        BOOST_TEST(test("x", alnum[ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
        BOOST_TEST(test("   A", alnum[ref(ch) = _1], space));
        BOOST_TEST(ch == 'A');
    }

    return boost::report_errors();
}
