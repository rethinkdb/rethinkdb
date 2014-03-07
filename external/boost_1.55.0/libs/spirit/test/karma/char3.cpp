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
    using boost::spirit::karma::lit;

    {
        BOOST_TEST(test("x", lit('x')));
        BOOST_TEST(!test("x", lit('y')));

        BOOST_TEST(test("x", lit('x'), 'x'));
        BOOST_TEST(!test("", lit('y'), 'x'));

//         BOOST_TEST(test("a", lit('a', 'z'), 'a'));
//         BOOST_TEST(test("b", lit('a', 'z'), 'b'));
//         BOOST_TEST(!test("", lit('a', 'z'), 'A'));

        BOOST_TEST(!test("", ~lit('x')));

        BOOST_TEST(!test("", ~lit('x'), 'x'));
        BOOST_TEST(test("x", ~lit('y'), 'x'));

//         BOOST_TEST(!test("", ~lit('a', 'z'), 'a'));
//         BOOST_TEST(!test("", ~lit('a', 'z'), 'b'));
//         BOOST_TEST(test("A", ~lit('a', 'z'), 'A'));

        BOOST_TEST(test("x", ~~lit('x')));
        BOOST_TEST(!test("x", ~~lit('y')));

        BOOST_TEST(test("x", ~~lit('x'), 'x'));
        BOOST_TEST(!test("", ~~lit('y'), 'x'));

//         BOOST_TEST(test("a", ~~lit('a', 'z'), 'a'));
//         BOOST_TEST(test("b", ~~lit('a', 'z'), 'b'));
//         BOOST_TEST(!test("", ~~lit('a', 'z'), 'A'));
    }

    {
        BOOST_TEST(test(L"x", lit('x')));
        BOOST_TEST(test(L"x", lit(L'x')));
        BOOST_TEST(!test(L"x", lit('y')));
        BOOST_TEST(!test(L"x", lit(L'y')));

        BOOST_TEST(test(L"x", lit(L'x'), L'x'));
        BOOST_TEST(!test(L"", lit('y'), L'x'));

//         BOOST_TEST(test("a", lit("a", "z"), 'a'));
//         BOOST_TEST(test(L"a", lit(L"a", L"z"), L'a'));

        BOOST_TEST(!test(L"", ~lit('x')));
        BOOST_TEST(!test(L"", ~lit(L'x')));

        BOOST_TEST(!test(L"", ~lit(L'x'), L'x'));
        BOOST_TEST(test(L"x", ~lit('y'), L'x'));
    }

    {   // lazy chars
        using namespace boost::phoenix;

        BOOST_TEST((test("x", lit(val('x')))));
        BOOST_TEST((test(L"x", lit(val(L'x')))));

        BOOST_TEST((test("x", lit(val('x')), 'x')));
        BOOST_TEST((test(L"x", lit(val(L'x')), L'x')));

        BOOST_TEST((!test("", lit(val('y')), 'x')));
        BOOST_TEST((!test(L"", lit(val(L'y')), L'x')));
    }

    // we can pass optionals as attributes to any generator
    {
        boost::optional<char> v;
        boost::optional<wchar_t> w;

        BOOST_TEST(!test("", lit('x'), v));
        BOOST_TEST(!test(L"", lit(L'x'), w));
    }

    return boost::report_errors();
}
