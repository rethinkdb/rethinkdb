//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/assign/std/vector.hpp>

#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <iostream>
#include <vector>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit::ascii;
    using boost::spirit::karma::repeat;
    using boost::spirit::karma::inf;
    using boost::spirit::karma::int_;
    using boost::spirit::karma::hex;
    using boost::spirit::karma::_1;

    {
        std::string str("aBcdeFGH");
        BOOST_TEST(test("abcdefgh", lower[repeat(8)[char_]], str));
        BOOST_TEST(test_delimited("A B C D E F G H ", upper[repeat(8)[char_]], str, space));
    }

   {
       std::string s1 = "aaaaa";
       BOOST_TEST(test("aaaaa", char_ << repeat(2)[char_ << char_], s1));
       s1 = "aaa";
       BOOST_TEST(test("aaa", char_ << repeat(1, 2)[char_ << char_], s1));
       s1 = "aa";
       BOOST_TEST(!test("", char_ << repeat(1)[char_ << char_], s1));
   }

    { // actions
        namespace phx = boost::phoenix;

        std::vector<char> v;
        v.push_back('a');
        v.push_back('a');
        v.push_back('a');
        v.push_back('a');
        BOOST_TEST(test("aaaa", repeat(4)[char_][_1 = phx::ref(v)]));
    }

    { // more actions
        namespace phx = boost::phoenix;

        std::vector<int> v;
        v.push_back(123);
        v.push_back(456);
        v.push_back(789);
        BOOST_TEST(test_delimited("123 456 789 ", repeat(3)[int_][_1 = phx::ref(v)], space));
    }

    // failing sub-generators
    {
        using boost::spirit::karma::strict;
        using boost::spirit::karma::relaxed;

        using namespace boost::assign;
        namespace karma = boost::spirit::karma;

        typedef std::pair<char, char> data;
        std::vector<data> v2, v3;
        v2 += std::make_pair('a', 'a'),
              std::make_pair('b', 'b'),
              std::make_pair('c', 'c'),
              std::make_pair('d', 'd'),
              std::make_pair('e', 'e'),
              std::make_pair('f', 'f'),
              std::make_pair('g', 'g');
        v3 += std::make_pair('a', 'a'),
              std::make_pair('b', 'b'),
              std::make_pair('c', 'c'),
              std::make_pair('d', 'd');

        karma::rule<spirit_test::output_iterator<char>::type, data()> r;

        r = &char_('d') << char_;
        BOOST_TEST(test("d", repeat[r], v2));
        BOOST_TEST(test("d", relaxed[repeat[r]], v2));
        BOOST_TEST(test("", strict[repeat[r]], v2));

        r = !char_('d') << char_;
        BOOST_TEST(test("abcefg", repeat(6)[r], v2));
        BOOST_TEST(!test("", repeat(5)[r], v2));
        BOOST_TEST(test("abcefg", relaxed[repeat(6)[r]], v2));
        BOOST_TEST(!test("", relaxed[repeat(5)[r]], v2));
        BOOST_TEST(!test("", strict[repeat(6)[r]], v2));
        BOOST_TEST(!test("", strict[repeat(5)[r]], v2));

        r = !char_('c') << char_;
        BOOST_TEST(test("abd", repeat(3)[r], v2));
        BOOST_TEST(test("abd", relaxed[repeat(3)[r]], v2));
        BOOST_TEST(!test("", strict[repeat(3)[r]], v2));

        r = !char_('a') << char_;
        BOOST_TEST(test("bcdef", repeat(3, 5)[r], v2));
        BOOST_TEST(test("bcd", repeat(3, 5)[r], v3));
        BOOST_TEST(!test("", repeat(4, 5)[r], v3));
        BOOST_TEST(test("bcdef", relaxed[repeat(3, 5)[r]], v2));
        BOOST_TEST(test("bcd", relaxed[repeat(3, 5)[r]], v3));
        BOOST_TEST(!test("", relaxed[repeat(4, 5)[r]], v3));
        BOOST_TEST(!test("", strict[repeat(3, 5)[r]], v2));
        BOOST_TEST(!test("", strict[repeat(3, 5)[r]], v3));
        BOOST_TEST(!test("", strict[repeat(4, 5)[r]], v3));

        BOOST_TEST(test("bcd", repeat(3, inf)[r], v3));
        BOOST_TEST(test("bcdefg", repeat(3, inf)[r], v2));
        BOOST_TEST(!test("", repeat(4, inf)[r], v3));

        r = !char_('g') << char_;
        BOOST_TEST(test("abcde", repeat(3, 5)[r], v2));
        BOOST_TEST(test("abcd", repeat(3, 5)[r], v3));
        BOOST_TEST(!test("", repeat(4, 5)[r], v3));
        BOOST_TEST(test("abcde", relaxed[repeat(3, 5)[r]], v2));
        BOOST_TEST(test("abcd", relaxed[repeat(3, 5)[r]], v3));
        BOOST_TEST(!test("", relaxed[repeat(4, 5)[r]], v3));
        BOOST_TEST(test("abcde", strict[repeat(3, 5)[r]], v2));
        BOOST_TEST(test("abcd", strict[repeat(3, 5)[r]], v3));
        BOOST_TEST(!test("", strict[repeat(5)[r]], v3));
    }

    return boost::report_errors();
}

