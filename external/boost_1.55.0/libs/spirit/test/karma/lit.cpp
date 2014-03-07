//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("a", lit('a')));
        BOOST_TEST(!test("a", lit('b')));

        BOOST_TEST(test("abc", "abc"));
        BOOST_TEST(!test("abcd", "abc"));

        BOOST_TEST(test("abc", lit("abc")));
        BOOST_TEST(!test("abcd", lit("abc")));

        BOOST_TEST(test("abc", string, "abc"));
        BOOST_TEST(!test("abcd", string, "abc"));

        BOOST_TEST(test("abc", string("abc")));
        BOOST_TEST(!test("abcd", string("abc")));

        BOOST_TEST(test("abc", string("abc"), "abc"));
        BOOST_TEST(!test("", string("abc"), "abcd"));
        BOOST_TEST(!test("", string("abcd"), "abc"));
        BOOST_TEST(!test("", string("abc"), "abcd"));   // don't match prefixes only
    }

    {
        using namespace boost::spirit::ascii;

        std::string str("abc");
        BOOST_TEST(test("abc", lit(str)));
        BOOST_TEST(!test("abcd", lit(str)));

        BOOST_TEST(test("abc", string(str)));
        BOOST_TEST(!test("abcd", string(str)));

        BOOST_TEST(test("abc", string, str));
        BOOST_TEST(!test("abcd", string, str));

        BOOST_TEST(test("abc", str));
        BOOST_TEST(!test("abcd", str));
    }

    {
        using namespace boost::spirit::standard_wide;

        std::basic_string<wchar_t> wstr(L"abc");
        BOOST_TEST(test(L"abc", lit(wstr)));
        BOOST_TEST(!test(L"abcd", lit(wstr)));

        BOOST_TEST(test(L"abc", string, wstr));
        BOOST_TEST(!test(L"abcd", string, wstr));

        BOOST_TEST(test(L"abc", wstr));
        BOOST_TEST(!test(L"abcd", wstr));
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test(L"a", lit(L'a')));
        BOOST_TEST(!test(L"a", lit(L'b')));

        BOOST_TEST(test(L"abc", L"abc"));
        BOOST_TEST(test(L"abc", "abc"));
        BOOST_TEST(!test(L"abcd", L"abc"));

        BOOST_TEST(test(L"abc", lit(L"abc")));
        BOOST_TEST(!test(L"abcd", lit(L"abc")));

        BOOST_TEST(test(L"abc", string(L"abc")));
        BOOST_TEST(!test(L"abcd", string(L"abc")));

        BOOST_TEST(test(L"abc", string, L"abc"));
        BOOST_TEST(!test(L"abcd", string, L"abc"));

        BOOST_TEST(test(L"abc", string, "abc"));
        BOOST_TEST(!test(L"abcd", string, "abc"));
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test_delimited("a ", lit('a'), ' '));
        BOOST_TEST(!test_delimited("a ", lit('b'), ' '));

        BOOST_TEST(test_delimited("abc ", "abc", ' '));
        BOOST_TEST(!test_delimited("abcd ", "abc", ' '));

        BOOST_TEST(test_delimited("abc ", lit("abc"), ' '));
        BOOST_TEST(!test_delimited("abcd ", lit("abc"), ' '));

        BOOST_TEST(test_delimited("abc ", string, "abc", ' '));
        BOOST_TEST(!test_delimited("abcd ", string, "abc", ' '));

        BOOST_TEST(test_delimited("abc ", string("abc"), ' '));
        BOOST_TEST(!test_delimited("abcd ", string("abc"), ' '));

        BOOST_TEST(test_delimited("abc ", string("abc"), "abc", ' '));
        BOOST_TEST(!test_delimited("", string("abc"), "abcd", ' '));
        BOOST_TEST(!test_delimited("", string("abcd"), "abc", ' '));
        BOOST_TEST(!test_delimited("", string("abc"), "abcd", ' '));   // don't match prefixes only
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test_delimited(L"a ", lit(L'a'), ' '));
        BOOST_TEST(!test_delimited(L"a ", lit(L'b'), ' '));

        BOOST_TEST(test_delimited(L"abc ", L"abc", ' '));
        BOOST_TEST(!test_delimited(L"abcd ", L"abc", ' '));

        BOOST_TEST(test_delimited(L"abc ", lit(L"abc"), ' '));
        BOOST_TEST(!test_delimited(L"abcd ", lit(L"abc"), ' '));

        BOOST_TEST(test_delimited(L"abc ", string, L"abc", ' '));
        BOOST_TEST(!test_delimited(L"abcd ", string, L"abc", ' '));

        BOOST_TEST(test_delimited(L"abc ", string(L"abc"), ' '));
        BOOST_TEST(!test_delimited(L"abcd ", string(L"abc"), ' '));
    }

    {   // test action
        namespace phx = boost::phoenix;
        using namespace boost::spirit::ascii;

        std::string str("abc");
        BOOST_TEST(test("abc", string[_1 = phx::ref(str)]));
        BOOST_TEST(test_delimited("abc ", string[_1 = phx::ref(str)], space));
    }

    {   // lazy strings
        namespace phx = boost::phoenix;
        using namespace boost::spirit::ascii;

        std::basic_string<char> s("abc");
        BOOST_TEST((test("abc", lit(phx::val(s)))));
        BOOST_TEST((test("abc", string(phx::val(s)))));

        BOOST_TEST(test("abc", string(phx::val(s)), "abc"));
        BOOST_TEST(!test("", string(phx::val(s)), "abcd"));
        BOOST_TEST(!test("", string(phx::val(s)), "abc"));

        std::basic_string<wchar_t> ws(L"abc");
        BOOST_TEST((test(L"abc", lit(phx::ref(ws)))));
        BOOST_TEST((test(L"abc", string(phx::ref(ws)))));
    }

    return boost::report_errors();
}
