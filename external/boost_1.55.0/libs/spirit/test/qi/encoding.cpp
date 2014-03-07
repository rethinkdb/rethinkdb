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
    using boost::spirit::qi::no_case;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::encoding;
    namespace char_encoding = boost::spirit::char_encoding;

    encoding<char_encoding::iso8859_1> iso8859_1;

// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("french")
#endif

    {
        BOOST_TEST(test("¡", iso8859_1[no_case['·']]));
        BOOST_TEST(test("¡", iso8859_1[no_case[char_('·')]]));
    }

    {
        BOOST_TEST(test("…", iso8859_1[no_case[char_("Â-Ô")]]));
        BOOST_TEST(!test("ˇ", iso8859_1[no_case[char_("Â-Ô")]]));
    }

    {
        BOOST_TEST(test("¡·", iso8859_1[no_case["·¡"]]));
        BOOST_TEST(test("¡·", iso8859_1[no_case[lit("·¡")]]));
    }


#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    return boost::report_errors();
}
