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

    { // synth attribute value-init

        std::string s;
        rule<char const*, char()> r;
        r = alpha[_val += _1];
        BOOST_TEST(test_attr("abcdef", +r, s));
        BOOST_TEST(s == "abcdef");
    }

    { // auto rules aliasing tests

        char ch = '\0';
        rule<char const*, char()> a, b;
        a %= b;
        b %= alpha;

        BOOST_TEST(test("x", a[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
        ch = '\0';
        BOOST_TEST(test_attr("z", a, ch)); // attribute is given.
        BOOST_TEST(ch == 'z');

        a = b;            // test deduced auto rule behavior
        b = alpha;

        ch = '\0';
        BOOST_TEST(test("x", a[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x');
        ch = '\0';
        BOOST_TEST(test_attr("z", a, ch)); // attribute is given.
        BOOST_TEST(ch == 'z');
    }

    { // context (w/arg) tests

        char ch;
        rule<char const*, char(int)> a; // 1 arg
        a = alpha[_val = _1 + _r1];

        BOOST_TEST(test("x", a(phx::val(1))[phx::ref(ch) = _1]));
        BOOST_TEST(ch == 'x' + 1);

        BOOST_TEST(test_attr("a", a(1), ch)); // allow scalars as rule args too.
        BOOST_TEST(ch == 'a' + 1);

        rule<char const*, char(int, int)> b; // 2 args
        b = alpha[_val = _1 + _r1 + _r2];
        BOOST_TEST(test_attr("a", b(1, 2), ch));
        BOOST_TEST(ch == 'a' + 1 + 2);
    }

    { // context (w/ reference arg) tests

        char ch;
        rule<char const*, void(char&)> a; // 1 arg (reference)
        a = alpha[_r1 = _1];

        BOOST_TEST(test("x", a(phx::ref(ch))));
        BOOST_TEST(ch == 'x');
    }

    { // context (w/locals) tests

        rule<char const*, locals<char> > a; // 1 local
        a = alpha[_a = _1] >> char_(_a);
        BOOST_TEST(test("aa", a));
        BOOST_TEST(!test("ax", a));
    }

    { // context (w/args and locals) tests

        rule<char const*, void(int), locals<char> > a; // 1 arg + 1 local
        a = alpha[_a = _1 + _r1] >> char_(_a);
        BOOST_TEST(test("ab", a(phx::val(1))));
        BOOST_TEST(test("xy", a(phx::val(1))));
        BOOST_TEST(!test("ax", a(phx::val(1))));
    }

    { // void() has unused type (void == unused_type)

        std::pair<int, char> attr;
        rule<char const*, void()> r;
        r = char_;
        BOOST_TEST(test_attr("123ax", int_ >> char_ >> r, attr));
        BOOST_TEST(attr.first == 123);
        BOOST_TEST(attr.second == 'a');
    }

    { // bug: test that injected attributes are ok

        rule<char const*, char(int) > r;

        // problem code:
        r = char_(_r1)[_val = _1];
    }

    return boost::report_errors();
}

