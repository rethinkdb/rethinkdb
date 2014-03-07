/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#define BOOST_SPIRIT_DEBUG

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
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
    using boost::spirit::qi::fail;
    using boost::spirit::qi::on_error;
    using boost::spirit::qi::debug;
    using boost::spirit::qi::alpha;

    namespace phx = boost::phoenix;

    { // basic tests

        rule<char const*> a, b, c, start;

        a = 'a';
        b = 'b';
        c = 'c';
        BOOST_SPIRIT_DEBUG_NODE(a);
        BOOST_SPIRIT_DEBUG_NODE(b);
        BOOST_SPIRIT_DEBUG_NODE(c);

        start = *(a | b | c);
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test("abcabcacb", start));

        start = (a | b) >> (start | b);
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test("aaaabababaaabbb", start));
        BOOST_TEST(test("aaaabababaaabba", start, false));

        // ignore the skipper!
        BOOST_TEST(test("aaaabababaaabba", start, space, false));
    }

    { // basic tests w/ skipper

        rule<char const*, space_type> a, b, c, start;

        a = 'a';
        b = 'b';
        c = 'c';
        BOOST_SPIRIT_DEBUG_NODE(a);
        BOOST_SPIRIT_DEBUG_NODE(b);
        BOOST_SPIRIT_DEBUG_NODE(c);

        start = *(a | b | c);
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test(" a b c a b c a c b ", start, space));

        start = (a | b) >> (start | b);
        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test(" a a a a b a b a b a a a b b b ", start, space));
        BOOST_TEST(test(" a a a a b a b a b a a a b b a ", start, space, false));
    }

    { // std::container attributes

        typedef boost::fusion::vector<int, char> fs;
        rule<char const*, std::vector<fs>(), space_type> start;
        start = *(int_ >> alpha);

        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_TEST(test("1 a 2 b 3 c", start, space));
    }

    { // error handling

        using namespace boost::spirit::ascii;
        using boost::phoenix::construct;
        using boost::phoenix::bind;

        rule<char const*> r;
        r = '(' > int_ > ',' > int_ > ')';

        on_error<fail>
        (
            r, std::cout
                << phx::val("Error! Expecting: ")
                << _4
                << phx::val(", got: \"")
                << construct<std::string>(_3, _2)
                << phx::val("\"")
                << std::endl
        );

        BOOST_SPIRIT_DEBUG_NODE(r);
        BOOST_TEST(test("(123,456)", r));
        BOOST_TEST(!test("(abc,def)", r));
        BOOST_TEST(!test("(123,456]", r));
        BOOST_TEST(!test("(123;456)", r));
        BOOST_TEST(!test("[123,456]", r));
    }

    return boost::report_errors();
}

