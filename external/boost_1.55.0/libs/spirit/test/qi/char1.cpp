/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using spirit_test::print_info;

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("x", 'x'));
        BOOST_TEST(test(L"x", L'x'));
        BOOST_TEST(!test("y", 'x'));
        BOOST_TEST(!test(L"y", L'x'));

        BOOST_TEST(test("x", char_));
        BOOST_TEST(test("x", char_('x')));
        BOOST_TEST(!test("x", char_('y')));
        BOOST_TEST(test("x", char_('a', 'z')));
        BOOST_TEST(!test("x", char_('0', '9')));

        BOOST_TEST(!test("x", ~char_));
        BOOST_TEST(!test("x", ~char_('x')));
        BOOST_TEST(test(" ", ~char_('x')));
        BOOST_TEST(test("X", ~char_('x')));
        BOOST_TEST(!test("x", ~char_('b', 'y')));
        BOOST_TEST(test("a", ~char_('b', 'y')));
        BOOST_TEST(test("z", ~char_('b', 'y')));

        BOOST_TEST(test("x", ~~char_));
        BOOST_TEST(test("x", ~~char_('x')));
        BOOST_TEST(!test(" ", ~~char_('x')));
        BOOST_TEST(!test("X", ~~char_('x')));
        BOOST_TEST(test("x", ~~char_('b', 'y')));
        BOOST_TEST(!test("a", ~~char_('b', 'y')));
        BOOST_TEST(!test("z", ~~char_('b', 'y')));
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("   x", 'x', space));
        BOOST_TEST(test(L"   x", L'x', space));

        BOOST_TEST(test("   x", char_, space));
        BOOST_TEST(test("   x", char_('x'), space));
        BOOST_TEST(!test("   x", char_('y'), space));
        BOOST_TEST(test("   x", char_('a', 'z'), space));
        BOOST_TEST(!test("   x", char_('0', '9'), space));
    }

    {
        using namespace boost::spirit::standard_wide;

        BOOST_TEST(test(L"x", char_));
        BOOST_TEST(test(L"x", char_(L'x')));
        BOOST_TEST(!test(L"x", char_(L'y')));
        BOOST_TEST(test(L"x", char_(L'a', L'z')));
        BOOST_TEST(!test(L"x", char_(L'0', L'9')));

        BOOST_TEST(!test(L"x", ~char_));
        BOOST_TEST(!test(L"x", ~char_(L'x')));
        BOOST_TEST(test(L" ", ~char_(L'x')));
        BOOST_TEST(test(L"X", ~char_(L'x')));
        BOOST_TEST(!test(L"x", ~char_(L'b', L'y')));
        BOOST_TEST(test(L"a", ~char_(L'b', L'y')));
        BOOST_TEST(test(L"z", ~char_(L'b', L'y')));

        BOOST_TEST(test(L"x", ~~char_));
        BOOST_TEST(test(L"x", ~~char_(L'x')));
        BOOST_TEST(!test(L" ", ~~char_(L'x')));
        BOOST_TEST(!test(L"X", ~~char_(L'x')));
        BOOST_TEST(test(L"x", ~~char_(L'b', L'y')));
        BOOST_TEST(!test(L"a", ~~char_(L'b', L'y')));
        BOOST_TEST(!test(L"z", ~~char_(L'b', L'y')));
    }


    {   // single char strings!
        namespace ascii = boost::spirit::ascii;
        namespace wide = boost::spirit::standard_wide;

        BOOST_TEST(test("x", "x"));
        BOOST_TEST(test(L"x", L"x"));
        BOOST_TEST(test("x", ascii::char_("x")));
        BOOST_TEST(test(L"x", wide::char_(L"x")));

        BOOST_TEST(test("x", ascii::char_("a", "z")));
        BOOST_TEST(test(L"x", ascii::char_(L"a", L"z")));
    }

    {
        // chsets
        namespace ascii = boost::spirit::ascii;
        namespace wide = boost::spirit::standard_wide;

        BOOST_TEST(test("x", ascii::char_("a-z")));
        BOOST_TEST(!test("1", ascii::char_("a-z")));
        BOOST_TEST(test("1", ascii::char_("a-z0-9")));

        BOOST_TEST(test("x", wide::char_(L"a-z")));
        BOOST_TEST(!test("1", wide::char_(L"a-z")));
        BOOST_TEST(test("1", wide::char_(L"a-z0-9")));

        std::string set = "a-z0-9";
        BOOST_TEST(test("x", ascii::char_(set)));

#ifdef SPIRIT_NO_COMPILE_CHECK
        test("", ascii::char_(L"a-z0-9"));
#endif
    }

    {   // lazy chars

        using namespace boost::spirit::ascii;

        using boost::phoenix::val;
        using boost::phoenix::ref;
        using boost::spirit::_1;

        BOOST_TEST((test("x", char_(val('x')))));
        BOOST_TEST((test("h", char_(val('a'), val('n')))));
        BOOST_TEST(test("0", char_(val("a-z0-9"))));

        char ch; // make sure lazy chars have an attribute
        BOOST_TEST(test("x", char_(val('x'))[ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
    }

    { // testing "what"

        using boost::spirit::qi::what;
        using boost::spirit::ascii::char_;
        using boost::spirit::ascii::alpha;

        print_info(what('x'));
        print_info(what(char_('a','z')));
        print_info(what(alpha));
    }

    return boost::report_errors();
}
