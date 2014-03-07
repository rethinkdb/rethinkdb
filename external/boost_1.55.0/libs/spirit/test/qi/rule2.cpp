/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// this file deliberately contains non-ascii characters
// boostinspect:noascii

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <cstring>
#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    using spirit_test::test;

    using namespace boost::spirit::ascii;
    using namespace boost::spirit::qi::labels;
    using boost::spirit::qi::locals;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::uint_;
    using boost::spirit::qi::fail;
    using boost::spirit::qi::on_error;
    using boost::spirit::qi::debug;
    using boost::spirit::qi::lit;

    namespace phx = boost::phoenix;

    { // test unassigned rule

        rule<char const*> a;
        BOOST_TEST(!test("x", a));
    }

    { // alias tests

        rule<char const*> a, b, c, d, start;

        a = 'a';
        b = 'b';
        c = 'c';
        d = start.alias(); // d will always track start

        start = *(a | b | c);
        BOOST_TEST(test("abcabcacb", d));

        start = (a | b) >> (start | b);
        BOOST_TEST(test("aaaabababaaabbb", d));
    }

    { // copy tests

        rule<char const*> a, b, c, start;

        a = 'a';
        b = 'b';
        c = 'c';

        // The FF is the dynamic equivalent of start = *(a | b | c);
        start = a;
        start = start.copy() | b;
        start = start.copy() | c;
        start = *(start.copy());

        BOOST_TEST(test("abcabcacb", start));

        // The FF is the dynamic equivalent of start = (a | b) >> (start | b);
        start = b;
        start = a | start.copy();
        start = start.copy() >> (start | b);

        BOOST_TEST(test("aaaabababaaabbb", start));
        BOOST_TEST(test("aaaabababaaabba", start, false));
    }

    { // context tests

        char ch;
        rule<char const*, char()> a;
        a = alpha[_val = _1];

        BOOST_TEST(test("x", a[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x');

        BOOST_TEST(test_attr("z", a, ch)); // attribute is given.
        BOOST_TEST(ch == 'z');
    }

    { // auto rules tests

        char ch = '\0';
        rule<char const*, char()> a;
        a %= alpha;

        BOOST_TEST(test("x", a[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
        ch = '\0';
        BOOST_TEST(test_attr("z", a, ch)); // attribute is given.
        BOOST_TEST(ch == 'z');

        a = alpha;    // test deduced auto rule behavior
        ch = '\0';
        BOOST_TEST(test("x", a[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
        ch = '\0';
        BOOST_TEST(test_attr("z", a, ch)); // attribute is given.
        BOOST_TEST(ch == 'z');
    }

    { // auto rules tests: allow stl containers as attributes to
      // sequences (in cases where attributes of the elements
      // are convertible to the value_type of the container or if
      // the element itself is an stl container with value_type
      // that is convertible to the value_type of the attribute).

        std::string s;
        rule<char const*, std::string()> r;
        r %= char_ >> *(',' >> char_);

        BOOST_TEST(test("a,b,c,d,e,f", r[phx::ref(s) = _1]));
        BOOST_TEST(s == "abcdef");

        r = char_ >> *(',' >> char_);    // test deduced auto rule behavior
        s.clear();
        BOOST_TEST(test("a,b,c,d,e,f", r[phx::ref(s) = _1]));
        BOOST_TEST(s == "abcdef");

        r %= char_ >> char_ >> char_ >> char_ >> char_ >> char_;
        s.clear();
        BOOST_TEST(test("abcdef", r[phx::ref(s) = _1]));
        BOOST_TEST(s == "abcdef");

        r = char_ >> char_ >> char_ >> char_ >> char_ >> char_;
        s.clear();
        BOOST_TEST(test("abcdef", r[phx::ref(s) = _1]));
        BOOST_TEST(s == "abcdef");
    }

    return boost::report_errors();
}

