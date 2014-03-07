/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// this file intentionally contains non-ascii characters
// boostinspect:noascii

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_directive.hpp>
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
    using boost::spirit::qi::lit;

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("x", no_case[char_]));
        BOOST_TEST(test("X", no_case[char_('x')]));
        BOOST_TEST(test("X", no_case[char_('X')]));
        BOOST_TEST(test("x", no_case[char_('X')]));
        BOOST_TEST(test("x", no_case[char_('x')]));
        BOOST_TEST(!test("z", no_case[char_('X')]));
        BOOST_TEST(!test("z", no_case[char_('x')]));
        BOOST_TEST(test("x", no_case[char_('a', 'z')]));
        BOOST_TEST(test("X", no_case[char_('a', 'z')]));
        BOOST_TEST(!test("a", no_case[char_('b', 'z')]));
        BOOST_TEST(!test("z", no_case[char_('a', 'y')]));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("X", no_case['x']));
        BOOST_TEST(test("X", no_case['X']));
        BOOST_TEST(test("x", no_case['X']));
        BOOST_TEST(test("x", no_case['x']));
        BOOST_TEST(!test("z", no_case['X']));
        BOOST_TEST(!test("z", no_case['x']));
    }

// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("french")
#endif
    {
        using namespace boost::spirit::iso8859_1;
        BOOST_TEST(test("¡", no_case[char_('·')]));
    }

    {
        using namespace boost::spirit::iso8859_1;
        BOOST_TEST(test("X", no_case[char_("a-z")]));
        BOOST_TEST(!test("1", no_case[char_("a-z")]));

        BOOST_TEST(test("…", no_case[char_("Â-Ô")]));
        BOOST_TEST(!test("ˇ", no_case[char_("Â-Ô")]));
    }
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("Bochi Bochi", no_case[lit("bochi bochi")]));
        BOOST_TEST(test("BOCHI BOCHI", no_case[lit("bochi bochi")]));
        BOOST_TEST(!test("Vavoo", no_case[lit("bochi bochi")]));
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("french")
#endif
    {
        using namespace boost::spirit::iso8859_1;
        BOOST_TEST(test("¡·", no_case[lit("·¡")]));
        BOOST_TEST(test("··", no_case[no_case[lit("·¡")]]));
    }
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    {
        // should work!
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("x", no_case[no_case[char_]]));
        BOOST_TEST(test("x", no_case[no_case[char_('x')]]));
        BOOST_TEST(test("yabadabadoo", no_case[no_case[lit("Yabadabadoo")]]));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST(test("X", no_case[alnum]));
        BOOST_TEST(test("6", no_case[alnum]));
        BOOST_TEST(!test(":", no_case[alnum]));

        BOOST_TEST(test("X", no_case[lower]));
        BOOST_TEST(test("x", no_case[lower]));
        BOOST_TEST(test("X", no_case[upper]));
        BOOST_TEST(test("x", no_case[upper]));
        BOOST_TEST(!test(":", no_case[lower]));
        BOOST_TEST(!test(":", no_case[upper]));
    }

    {
        using namespace boost::spirit::iso8859_1;
        BOOST_TEST(test("X", no_case[alnum]));
        BOOST_TEST(test("6", no_case[alnum]));
        BOOST_TEST(!test(":", no_case[alnum]));

        BOOST_TEST(test("X", no_case[lower]));
        BOOST_TEST(test("x", no_case[lower]));
        BOOST_TEST(test("X", no_case[upper]));
        BOOST_TEST(test("x", no_case[upper]));
        BOOST_TEST(!test(":", no_case[lower]));
        BOOST_TEST(!test(":", no_case[upper]));
    }

    {
        using namespace boost::spirit::standard;
        BOOST_TEST(test("X", no_case[alnum]));
        BOOST_TEST(test("6", no_case[alnum]));
        BOOST_TEST(!test(":", no_case[alnum]));

        BOOST_TEST(test("X", no_case[lower]));
        BOOST_TEST(test("x", no_case[lower]));
        BOOST_TEST(test("X", no_case[upper]));
        BOOST_TEST(test("x", no_case[upper]));
        BOOST_TEST(!test(":", no_case[lower]));
        BOOST_TEST(!test(":", no_case[upper]));
    }

    {
        // chsets
        namespace standard = boost::spirit::standard;
        namespace standard_wide = boost::spirit::standard_wide;

        BOOST_TEST(test("x", standard::no_case[standard::char_("a-z")]));
        BOOST_TEST(test("X", standard::no_case[standard::char_("a-z")]));
        BOOST_TEST(test(L"X", standard_wide::no_case[standard_wide::char_(L"a-z")]));
        BOOST_TEST(test(L"X", standard_wide::no_case[standard_wide::char_(L"X")]));
    }

    {
        using namespace boost::spirit::standard;
        std::string s("bochi bochi");
        BOOST_TEST(test("Bochi Bochi", no_case[lit(s.c_str())]));
        BOOST_TEST(test("Bochi Bochi", no_case[lit(s)]));
        BOOST_TEST(test("Bochi Bochi", no_case[s.c_str()]));
        BOOST_TEST(test("Bochi Bochi", no_case[s]));
    }

    {   // lazy no_case chars

        using namespace boost::spirit::ascii;

        using boost::phoenix::val;
        using boost::phoenix::ref;
        using boost::spirit::_1;

        BOOST_TEST((test("X", no_case[val('x')])));
        BOOST_TEST((test("h", no_case[char_(val('a'), val('n'))])));
        BOOST_TEST(test("0", no_case[char_(val("a-z0-9"))]));

        char ch; // make sure lazy chars have an attribute
        BOOST_TEST(test("x", no_case[char_(val('x'))][ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
    }

    {   // lazy no_case lits

        using namespace boost::spirit::ascii;
        using boost::phoenix::val;

        BOOST_TEST(test("Bochi Bochi", no_case[val("bochi bochi")]));
        BOOST_TEST(test("BOCHI BOCHI", no_case[val("bochi bochi")]));
        BOOST_TEST(!test("Vavoo", no_case[val("bochi bochi")]));
    }

    return boost::report_errors();
}
