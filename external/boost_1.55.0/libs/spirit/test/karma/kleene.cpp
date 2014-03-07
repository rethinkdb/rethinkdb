//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/assign/std/vector.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <boost/assign/std/vector.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
struct action 
{
    action (std::vector<char>& vec) 
      : vec(vec), it(vec.begin()) 
    {}

    void operator()(unsigned& value, boost::spirit::unused_type, bool& pass) const
    {
       pass = (it != vec.end());
       if (pass)
           value = *it++;
    }

    std::vector<char>& vec;
    mutable std::vector<char>::iterator it;
};

struct A
{
    double d1;
    double d2;
};

BOOST_FUSION_ADAPT_STRUCT(
    A,
    (double, d1)
    (double, d2)
)

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;
    namespace fusion = boost::fusion;

    {
        std::string s1("aaaa");
        BOOST_TEST(test("aaaa", *char_, s1));
        BOOST_TEST(test_delimited("a a a a ", *char_, s1, ' '));

        std::string s2("");
        BOOST_TEST(test("", *char_, s2));
        BOOST_TEST(test_delimited("", *char_, s2, ' '));
    }

    {
        std::string s1("aaaaa");
        BOOST_TEST(test("aaaaa", char_ << *(char_ << char_), s1));
        BOOST_TEST(test_delimited("a a a a a ", 
            char_ << *(char_ << char_), s1, ' '));

        s1 = "a";
        BOOST_TEST(test("a", char_ << *(char_ << char_), s1));
        s1 = "aa";
        BOOST_TEST(!test("", char_ << *(char_ << char_), s1));
//         BOOST_TEST(test("aa", char_ << *buffer[char_ << char_] << char_, s1));
        s1 = "aaaa";
        BOOST_TEST(!test("", char_ << *(char_ << char_), s1));
//         BOOST_TEST(test("aaaa", char_ << *buffer[char_ << char_] << char_, s1));
    }

    {
        using boost::spirit::karma::strict;
        using boost::spirit::karma::relaxed;
        using namespace boost::assign;

        typedef std::pair<char, char> data;
        std::vector<data> v1;
        v1 += std::make_pair('a', 'a'), 
              std::make_pair('b', 'b'), 
              std::make_pair('c', 'c'), 
              std::make_pair('d', 'd'), 
              std::make_pair('e', 'e'), 
              std::make_pair('f', 'f'), 
              std::make_pair('g', 'g'); 

        karma::rule<spirit_test::output_iterator<char>::type, data()> r;
        r = &char_('a') << char_;

        BOOST_TEST(test("a", r << *(r << r), v1));
        BOOST_TEST(test("a", relaxed[r << *(r << r)], v1));
        BOOST_TEST(!test("", strict[r << *(r << r)], v1));

         v1 += std::make_pair('a', 'a');

        BOOST_TEST(!test("", r << *(r << r), v1));
        BOOST_TEST(!test("", relaxed[r << *(r << r)], v1));
        BOOST_TEST(!test("", strict[r << *(r << r)], v1));

         v1 += std::make_pair('a', 'a');

        BOOST_TEST(test("aaa", r << *(r << r), v1));
        BOOST_TEST(test("aaa", relaxed[r << *(r << r)], v1));
        BOOST_TEST(!test("", strict[r << *(r << r)], v1));
    }

    {
        using namespace boost::assign;

        std::vector<char> v;
        v += 'a', 'b', 'c';

        BOOST_TEST(test("abc", *char_, v));
        BOOST_TEST(test_delimited("a b c ", *char_, v, ' '));
    }

    {
        using namespace boost::assign;

        std::vector<int> v;
        v += 10, 20, 30;

        BOOST_TEST(test("102030", *int_, v));
        BOOST_TEST(test_delimited("10, 20, 30, ", *int_, v, lit(", ")));

        BOOST_TEST(test("10,20,30,", *(int_ << ','), v));
        BOOST_TEST(test_delimited("10 , 20 , 30 , ", *(int_ << ','), v, lit(' ')));

// leads to infinite loops
//         fusion::vector<char, char> cc ('a', 'c');
//         BOOST_TEST(test("ac", char_ << *(lit(' ') << ',') << char_, cc));
//         BOOST_TEST(test_delimited("a c ", 
//             char_ << *(lit(' ') << ',') << char_, cc, " "));
    }

    { // actions
        using namespace boost::assign;
        namespace phx = boost::phoenix;

        std::vector<char> v;
        v += 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h';

        BOOST_TEST(test("abcdefgh", (*char_)[_1 = phx::ref(v)]));
        BOOST_TEST(test_delimited("a b c d e f g h ", 
            (*char_ )[_1 = phx::ref(v)], space));
    }

    // failing sub-generators
    {
        using boost::spirit::karma::strict;
        using boost::spirit::karma::relaxed;

        using namespace boost::assign;

        typedef std::pair<char, char> data;
        std::vector<data> v2;
        v2 += std::make_pair('a', 'a'), 
              std::make_pair('b', 'b'), 
              std::make_pair('c', 'c'), 
              std::make_pair('d', 'd'), 
              std::make_pair('e', 'e'), 
              std::make_pair('f', 'f'), 
              std::make_pair('g', 'g');

        karma::rule<spirit_test::output_iterator<char>::type, data()> r;

        r = &char_('d') << char_;
        BOOST_TEST(test("d", *r, v2));
        BOOST_TEST(test("d", relaxed[*r], v2));
        BOOST_TEST(test("", strict[*r], v2));

        r = &char_('a') << char_;
        BOOST_TEST(test("a", *r, v2));
        BOOST_TEST(test("a", relaxed[*r], v2));
        BOOST_TEST(test("a", strict[*r], v2));

        r = &char_('g') << char_;
        BOOST_TEST(test("g", *r, v2));
        BOOST_TEST(test("g", relaxed[*r], v2));
        BOOST_TEST(test("", strict[*r], v2));

        r = !char_('d') << char_;
        BOOST_TEST(test("abcefg", *r, v2));
        BOOST_TEST(test("abcefg", relaxed[*r], v2));
        BOOST_TEST(test("abc", strict[*r], v2));

        r = !char_('a') << char_;
        BOOST_TEST(test("bcdefg", *r, v2));
        BOOST_TEST(test("bcdefg", relaxed[*r], v2));
        BOOST_TEST(test("", strict[*r], v2));

        r = !char_('g') << char_;
        BOOST_TEST(test("abcdef", *r, v2));
        BOOST_TEST(test("abcdef", relaxed[*r], v2));
        BOOST_TEST(test("abcdef", strict[*r], v2));

        r = &char_('A') << char_;
        BOOST_TEST(test("", *r, v2));
    }

    {
        // make sure user defined end condition is applied if no attribute
        // is passed in
        using namespace boost::assign;

        std::vector<char> v;
        v += 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h';
        BOOST_TEST(test("[6162636465666768]", '[' << *hex[action(v)] << ']'));
    }

    {
        using boost::spirit::karma::double_;

        std::vector<A> v(1);
        v[0].d1 = 1.0;
        v[0].d2 = 2.0;
        BOOST_TEST(test("A1.02.0", 'A' << *(double_ << double_), v));
    }

    return boost::report_errors();
}

