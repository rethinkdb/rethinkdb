//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this file deliberately contains non-ascii characters
// boostinspect:noascii

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    typedef spirit_test::output_iterator<char>::type outiter_type;

    {
        karma::rule<outiter_type, void(char, int, double)> start;
        fusion::vector<char, int, double> vec('a', 10, 12.4);

        start = char_[_1 = _r1] << int_[_1 = _r2] << double_[_1 = _r3];
        BOOST_TEST(test("a1012.4", start('a', 10, 12.4)));

        start = (char_ << int_ << double_)[_1 = _r1, _2 = _r2, _3 = _r3];
        BOOST_TEST(test("a1012.4", start('a', 10, 12.4)));

        karma::rule<outiter_type, void(char)> a;
        karma::rule<outiter_type, void(int)> b;
        karma::rule<outiter_type, void(double)> c;

        a = char_[_1 = _r1];
        b = int_[_1 = _r1];
        c = double_[_1 = _r1];
        start = a(_r1) << b(_r2) << c(_r3);
        BOOST_TEST(test("a1012.4", start('a', 10, 12.4)));
    }

    {
        karma::rule<outiter_type, space_type, void(char, int, double)> start;
        fusion::vector<char, int, double> vec('a', 10, 12.4);

        start = char_[_1 = _r1] << int_[_1 = _r2] << double_[_1 = _r3];
        BOOST_TEST(test_delimited("a 10 12.4 ", start('a', 10, 12.4), space));

        start = (char_ << int_ << double_)[_1 = _r1, _2 = _r2, _3 = _r3];
        BOOST_TEST(test_delimited("a 10 12.4 ", start('a', 10, 12.4), space));

        karma::rule<outiter_type, space_type, void(char)> a;
        karma::rule<outiter_type, space_type, void(int)> b;
        karma::rule<outiter_type, space_type, void(double)> c;

        a = char_[_1 = _r1];
        b = int_[_1 = _r1];
        c = double_[_1 = _r1];
        start = a(_r1) << b(_r2) << c(_r3);
        BOOST_TEST(test_delimited("a 10 12.4 ", start('a', 10, 12.4), space));
    }

    // copy tests
    {
        typedef variant<char, int, double> var_type;

        karma::rule<outiter_type> a, b, c, start;

        a = 'a';
        b = int_(10);
        c = double_(12.4);

        // The FF is the dynamic equivalent of start = a << b << c;
        start = a;
        start = start.copy() << b;
        start = start.copy() << c;
        start = start.copy();

        BOOST_TEST(test("a1012.4", start));
    }

    {
        typedef variant<char, int, double> var_type;

        karma::rule<outiter_type, space_type> a, b, c, start;

        a = 'a';
        b = int_(10);
        c = double_(12.4);

        // The FF is the dynamic equivalent of start = a << b << c;
        start = a;
        start = start.copy() << b;
        start = start.copy() << c;
        start = start.copy();

        BOOST_TEST(test_delimited("a 10 12.4 ", start, space));
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("french")
#endif
    { // specifying the encoding
        using karma::lower;
        using karma::upper;
        using karma::string;

        typedef boost::spirit::char_encoding::iso8859_1 iso8859_1;
        karma::rule<outiter_type, iso8859_1> r;

        r = lower['·'];
        BOOST_TEST(test("·", r));
        r = lower[char_('¡')];
        BOOST_TEST(test("·", r));
        r = upper['·'];
        BOOST_TEST(test("¡", r));
        r = upper[char_('¡')];
        BOOST_TEST(test("¡", r));

        r = lower["·¡"];
        BOOST_TEST(test("··", r));
        r = lower[lit("·¡")];
        BOOST_TEST(test("··", r));
        r = lower[string("·¡")];
        BOOST_TEST(test("··", r));
        r = upper["·¡"];
        BOOST_TEST(test("¡¡", r));
        r = upper[lit("·¡")];
        BOOST_TEST(test("¡¡", r));
        r = upper[string("·¡")];
        BOOST_TEST(test("¡¡", r));
    }

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    return boost::report_errors();
}

