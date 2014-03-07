//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2001-2010 Joel de Guzman
//  Copyright (c) 2011 Aaron Graham
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_binary.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <boost/spirit/repository/include/qi_advance.hpp>

#include <boost/assign/std/list.hpp>
#include "test.hpp"

namespace spirit_test
{
    template <typename Container, typename Parser>
    bool test_c(Container const& in, Parser const& p, bool full_match = true)
    {
        // we don't care about the results of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        typename Container::const_iterator first = in.begin();
        typename Container::const_iterator const last = in.end();
        return boost::spirit::qi::parse(first, last, p)
            && (!full_match || (first == last));
    }
}

int main()
{
    using spirit_test::test;
    using spirit_test::test_c;

    using namespace boost::spirit::qi::labels;
    using boost::spirit::qi::locals;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::uint_;
    using boost::spirit::qi::byte_;

    using namespace boost::assign;
    using boost::spirit::repository::qi::advance;

    { // test basic functionality with random-access iterators
        rule<char const*> start;

        start = 'a' >> advance(3) >> "bc";
        BOOST_TEST(test("a123bc", start));

        start = (advance(3) | 'q') >> 'i';
        BOOST_TEST(test("qi", start));

        start = advance(-1);
        BOOST_TEST(!test("0", start));

        start = advance(-1) | "qi";
        BOOST_TEST(test("qi", start));

        start = advance(0) >> "abc" >> advance(10) >> "nopq" >> advance(0)
            >> advance(8) >> 'z';
        BOOST_TEST(test("abcdefghijklmnopqrstuvwxyz", start));
    }

    { // test locals
        rule<char const*, locals<unsigned> > start;

        start = byte_ [_a = _1] >> advance(_a) >> "345";
        BOOST_TEST(test("\x02""12345", start));
        BOOST_TEST(!test("\x60""345", start));
    }

    { // test basic functionality with bidirectional iterators
        rule<std::list<char>::const_iterator, locals<int> > start;
        std::list<char> list;

        list.clear();
        list += 1,2,'a','b','c';
        start = byte_ [_a = _1] >> advance(_a) >> "abc";
        BOOST_TEST(test_c(list, start));

        list.clear();
        list += 3,'q','i';
        start = byte_ [_a = _1] >> advance(_a);
        BOOST_TEST(!test_c(list, start));

        start = byte_ [_a = _1] >> (advance(_a) | "qi");
        BOOST_TEST(test_c(list, start));

        list.clear();
        list += 'a','b','c','d','e','f','g','h','i','j','k','l','m';
        list += 'n','o','p','q','r','s','t','u','v','w','x','y','z';
        start = advance(0) >> "abc" >> advance(10) >> "nopq" >> advance(0)
            >> advance(8) >> 'z';
        BOOST_TEST(test_c(list, start));
    }

    return boost::report_errors();
}

