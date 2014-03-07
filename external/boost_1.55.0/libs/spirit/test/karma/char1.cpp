//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define KARMA_FAIL_COMPILATION

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;
    using namespace boost::phoenix;

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("x", 'x'));
        BOOST_TEST(test(L"x", L'x'));
        BOOST_TEST(!test("x", 'y'));
        BOOST_TEST(!test(L"x", L'y'));

        BOOST_TEST(test("x", "x"));
        BOOST_TEST(test(L"x", L"x"));
        BOOST_TEST(!test("x", "y"));
        BOOST_TEST(!test(L"x", L"y"));

        BOOST_TEST(test("x", char_, 'x'));
        BOOST_TEST(test(L"x", char_, L'x'));
        BOOST_TEST(!test("x", char_, 'y'));
        BOOST_TEST(!test(L"x", char_, L'y'));

        BOOST_TEST(test("x", char_('x')));
        BOOST_TEST(!test("x", char_('y')));

        BOOST_TEST(test("x", char_('x'), 'x'));
        BOOST_TEST(!test("", char_('y'), 'x'));

        BOOST_TEST(test("x", char_("x")));

        BOOST_TEST(test("a", char_('a', 'z'), 'a'));
        BOOST_TEST(test("b", char_('a', 'z'), 'b'));
        BOOST_TEST(!test("", char_('a', 'z'), 'A'));

        BOOST_TEST(test("a", char_("a-z"), 'a'));
        BOOST_TEST(test("b", char_("a-z"), 'b'));
        BOOST_TEST(!test("", char_("a-z"), 'A'));

#if defined(KARMA_FAIL_COMPILATION)
        BOOST_TEST(test("x", char_));           // anychar without a parameter doesn't make any sense
        BOOST_TEST(test("", char_('a', 'z')));  // char sets without attribute neither
#endif

        BOOST_TEST(!test("", ~char_('x')));

        BOOST_TEST(!test("", ~char_('x'), 'x'));
        BOOST_TEST(test("x", ~char_('y'), 'x'));

        BOOST_TEST(!test("", ~char_("x")));

        BOOST_TEST(!test("", ~char_('a', 'z'), 'a'));
        BOOST_TEST(!test("", ~char_('a', 'z'), 'b'));
        BOOST_TEST(test("A", ~char_('a', 'z'), 'A'));

        BOOST_TEST(!test("", ~char_("a-z"), 'a'));
        BOOST_TEST(!test("", ~char_("a-z"), 'b'));
        BOOST_TEST(test("A", ~char_("a-z"), 'A'));

        BOOST_TEST(test("x", ~~char_('x')));
        BOOST_TEST(!test("x", ~~char_('y')));

        BOOST_TEST(test("x", ~~char_('x'), 'x'));
        BOOST_TEST(!test("", ~~char_('y'), 'x'));

        BOOST_TEST(test("x", ~~char_("x")));

        BOOST_TEST(test("a", ~~char_('a', 'z'), 'a'));
        BOOST_TEST(test("b", ~~char_('a', 'z'), 'b'));
        BOOST_TEST(!test("", ~~char_('a', 'z'), 'A'));

        BOOST_TEST(test("a", ~~char_("a-z"), 'a'));
        BOOST_TEST(test("b", ~~char_("a-z"), 'b'));
        BOOST_TEST(!test("", ~~char_("a-z"), 'A'));
    }

    {
        using namespace boost::spirit::standard_wide;

        BOOST_TEST(test(L"x", 'x'));
        BOOST_TEST(test(L"x", L'x'));
        BOOST_TEST(!test(L"x", 'y'));
        BOOST_TEST(!test(L"x", L'y'));

        BOOST_TEST(test(L"x", "x"));
        BOOST_TEST(test(L"x", L"x"));
        BOOST_TEST(!test(L"x", "y"));
        BOOST_TEST(!test(L"x", L"y"));

        BOOST_TEST(test(L"x", char_, 'x'));
        BOOST_TEST(test(L"x", char_, L'x'));
        BOOST_TEST(!test(L"x", char_, 'y'));
        BOOST_TEST(!test(L"x", char_, L'y'));

        BOOST_TEST(test(L"x", char_('x')));
        BOOST_TEST(test(L"x", char_(L'x')));
        BOOST_TEST(!test(L"x", char_('y')));
        BOOST_TEST(!test(L"x", char_(L'y')));

        BOOST_TEST(test(L"x", char_(L'x'), L'x'));
        BOOST_TEST(!test(L"", char_('y'), L'x'));

        BOOST_TEST(test(L"x", char_(L"x")));

        BOOST_TEST(test("a", char_("a", "z"), 'a'));
        BOOST_TEST(test(L"a", char_(L"a", L"z"), L'a'));

#if defined(KARMA_FAIL_COMPILATION)
        BOOST_TEST(test("x", char_));           // anychar without a parameter doesn't make any sense
#endif

        BOOST_TEST(!test(L"", ~char_('x')));
        BOOST_TEST(!test(L"", ~char_(L'x')));

        BOOST_TEST(!test(L"", ~char_(L'x'), L'x'));
        BOOST_TEST(test(L"x", ~char_('y'), L'x'));

        BOOST_TEST(!test(L"", ~char_(L"x")));
    }

    {   // lazy chars
        namespace ascii = boost::spirit::ascii;
        namespace wide = boost::spirit::standard_wide;

        using namespace boost::phoenix;

        BOOST_TEST((test("x", ascii::char_(val('x')))));
        BOOST_TEST((test(L"x", wide::char_(val(L'x')))));

        BOOST_TEST((test("x", ascii::char_(val('x')), 'x')));
        BOOST_TEST((test(L"x", wide::char_(val(L'x')), L'x')));

        BOOST_TEST((!test("", ascii::char_(val('y')), 'x')));
        BOOST_TEST((!test(L"", wide::char_(val(L'y')), L'x')));
    }

    // we can pass optionals as attributes to any generator
    {
        namespace ascii = boost::spirit::ascii;
        namespace wide = boost::spirit::standard_wide;

        boost::optional<char> v;
        boost::optional<wchar_t> w;

        BOOST_TEST(!test("", ascii::char_, v));
        BOOST_TEST(!test(L"", wide::char_, w));

        BOOST_TEST(!test("", ascii::char_('x'), v));
        BOOST_TEST(!test(L"", wide::char_(L'x'), w));
    }

    return boost::report_errors();
}
