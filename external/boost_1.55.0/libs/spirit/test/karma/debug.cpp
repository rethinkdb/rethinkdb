//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_SPIRIT_KARMA_DEBUG

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <cstring>
#include <iostream>
#include "test.hpp"

int main()
{
    using spirit_test::test;
    using spirit_test::test_delimited;

    using namespace boost::spirit::ascii;
    using namespace boost::spirit::karma::labels;
    using boost::spirit::karma::locals;
    using boost::spirit::karma::rule;
    using boost::spirit::karma::char_;
    using boost::spirit::karma::debug;
    using boost::spirit::karma::space;
    using boost::spirit::karma::eps;

    namespace phx = boost::phoenix;

    typedef spirit_test::output_iterator<char>::type outiter_type;

    { // basic tests
        rule<outiter_type, char()> a, b, c;
        rule<outiter_type, std::vector<char>()> start;

        std::vector<char> v;
        v.push_back('a');
        v.push_back('b');
        v.push_back('a');
        v.push_back('c');
        v.push_back('a');
        v.push_back('b');
        v.push_back('b');
        v.push_back('a');

        a = char_('a');
        b = char_('b');
        c = char_('c');
        BOOST_SPIRIT_DEBUG_NODE(a);
        BOOST_SPIRIT_DEBUG_NODE(b);
        BOOST_SPIRIT_DEBUG_NODE(c);

        start = *(a | b | c);
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test("abacabba", start, v));

        // ignore the delimiter
        BOOST_TEST(test_delimited("abacabba ", start, v, space));

        std::vector<char> v1;
        v1.push_back('b');
        v1.push_back('c');

        start = (a | b) << c;
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test("bc", start, v1));
    }

    { // tests with locals
        rule<outiter_type, char()> a, b, c;
        rule<outiter_type, std::vector<char>(), locals<int, double> > start;

        std::vector<char> v;
        v.push_back('a');
        v.push_back('b');
        v.push_back('a');
        v.push_back('c');
        v.push_back('a');
        v.push_back('b');
        v.push_back('b');
        v.push_back('a');

        a = char_('a');
        b = char_('b');
        c = char_('c');
        BOOST_SPIRIT_DEBUG_NODE(a);
        BOOST_SPIRIT_DEBUG_NODE(b);
        BOOST_SPIRIT_DEBUG_NODE(c);

        start %= eps[_a = 0, _b = 2.0] << *(a[++_a] | b | c);
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test("abacabba", start, v));
    }

    return boost::report_errors();
}

