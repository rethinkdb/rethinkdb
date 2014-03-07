/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <string>
#include <iostream>
#include "test.hpp"

using spirit_test::test;
using spirit_test::test_attr;

using boost::spirit::ascii::space_type;
using boost::spirit::ascii::space;
using boost::spirit::int_;
using boost::spirit::qi::grammar;
using boost::spirit::qi::rule;
using boost::spirit::_val;
using boost::spirit::_r1;
using boost::spirit::lit;

struct num_list : grammar<char const*, space_type>
{
    num_list() : base_type(start)
    {
        using boost::spirit::int_;
        num = int_;
        start = num >> *(',' >> num);
    }

    rule<char const*, space_type> start, num;
};

struct inh_g : grammar<char const*, int(int), space_type>
{
    inh_g() : base_type(start)
    {
        start = lit("inherited")[_val = _r1];
    }

    rule<char const*, int(int), space_type> start, num;
};

struct my_skipper : grammar<char const*>
{
    my_skipper() : base_type(start)
    {
        start = space;
    }

    rule<char const*> start, num;
};

struct num_list2 : grammar<char const*, my_skipper>
{
    num_list2() : base_type(start)
    {
        using boost::spirit::int_;
        num = int_;
        start = num >> *(',' >> num);
    }

    rule<char const*, my_skipper> start, num;
};

template <typename Iterator, typename Skipper>
struct num_list3 : grammar<Iterator, Skipper>
{
    template <typename Class>
    num_list3(Class&) : grammar<Iterator, Skipper>(start)
    {
        using boost::spirit::int_;
        num = int_;
        start = num >> *(',' >> num);
    }

    rule<Iterator, Skipper> start, num;
};

int
main()
{
    { // simple grammar test

        num_list nlist;
        BOOST_TEST(test("123, 456, 789", nlist, space));
    }

    { // simple grammar test with user-skipper

        num_list2 nlist;
        my_skipper skip;
        BOOST_TEST(test("123, 456, 789", nlist, skip));
    }

    { // direct access to the rules

        num_list g;
        BOOST_TEST(test("123", g.num, space));
        BOOST_TEST(test("123, 456, 789", g.start, space));
    }

    { // grammar with inherited attributes

        inh_g g;
        int n = -1;
        BOOST_TEST(test_attr("inherited", g.start(123), n, space)); // direct to the rule
        BOOST_TEST(n == 123);
        BOOST_TEST(test_attr("inherited", g(123), n, space)); // using the grammar
        BOOST_TEST(n == 123);
    }

    return boost::report_errors();
}

