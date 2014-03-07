/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/phoenix_core.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using boost::spirit::qi::skip;
    using boost::spirit::qi::lexeme;
    using boost::spirit::qi::lit;
    using boost::spirit::ascii::char_;
    using boost::spirit::ascii::space;
    using boost::spirit::ascii::alpha;

    {
        BOOST_TEST((test("a b c d", skip(space)[*char_])));
    }

    { // test attribute
        std::string s;
        BOOST_TEST((test_attr("a b c d", skip(space)[*char_], s)));
        BOOST_TEST(s == "abcd");
    }

    { // reskip
        BOOST_TEST((test("ab c d", lexeme[lit('a') >> 'b' >> skip[lit('c') >> 'd']], space)));
        BOOST_TEST((test("abcd", lexeme[lit('a') >> 'b' >> skip[lit('c') >> 'd']], space)));
        BOOST_TEST(!(test("a bcd", lexeme[lit('a') >> 'b' >> skip[lit('c') >> 'd']], space)));
    }

    { // lazy skip
        using boost::phoenix::val;

        BOOST_TEST((test("a b c d", skip(val(space))[*char_])));
    }

    return boost::report_errors();
}
