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

    { // basic tests

        rule<char const*> a, b, c, start;

        a = 'a';
        b = 'b';
        c = 'c';

        a.name("a");
        b.name("b");
        c.name("c");
        start.name("start");

        debug(a);
        debug(b);
        debug(c);
        debug(start);

        start = *(a | b | c);
        BOOST_TEST(test("abcabcacb", start));

        start = (a | b) >> (start | b);
        BOOST_TEST(test("aaaabababaaabbb", start));
        BOOST_TEST(test("aaaabababaaabba", start, false));

        // ignore the skipper!
        BOOST_TEST(test("aaaabababaaabba", start, space, false));
    }

    { // basic tests with direct initialization

        rule<char const*> a ('a');
        rule<char const*> b ('b');
        rule<char const*> c ('c');
        rule<char const*> start = (a | b) >> (start | b);

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

        a.name("a");
        b.name("b");
        c.name("c");
        start.name("start");

        debug(a);
        debug(b);
        debug(c);
        debug(start);

        start = *(a | b | c);
        BOOST_TEST(test(" a b c a b c a c b ", start, space));

        start = (a | b) >> (start | b);
        BOOST_TEST(test(" a a a a b a b a b a a a b b b ", start, space));
        BOOST_TEST(test(" a a a a b a b a b a a a b b a ", start, space, false));
    }

    { // basic tests w/ skipper but no final post-skip

        rule<char const*, space_type> a, b, c, start;

        a = 'a';
        b = 'b';
        c = 'c';

        a.name("a");
        b.name("b");
        c.name("c");
        start.name("start");

        debug(a);
        debug(b);
        debug(c);
        debug(start);

        start = *(a | b) >> c;

        using boost::spirit::qi::phrase_parse;
        using boost::spirit::qi::skip_flag;
        {
            char const *s1 = " a b a a b b a c ... "
              , *const e1 = s1 + std::strlen(s1);
            BOOST_TEST(phrase_parse(s1, e1, start, space, skip_flag::dont_postskip)
              && s1 == e1 - 5);
        }

        start = (a | b) >> (start | c);
        {
            char const *s1 = " a a a a b a b a b a a a b b b c "
              , *const e1 = s1 + std::strlen(s1);
            BOOST_TEST(phrase_parse(s1, e1, start, space, skip_flag::postskip)
              && s1 == e1);
        }
        {
            char const *s1 = " a a a a b a b a b a a a b b b c "
              , *const e1 = s1 + std::strlen(s1);
            BOOST_TEST(phrase_parse(s1, e1, start, space, skip_flag::dont_postskip)
              && s1 == e1 - 1);
        }
    }

    return boost::report_errors();
}

