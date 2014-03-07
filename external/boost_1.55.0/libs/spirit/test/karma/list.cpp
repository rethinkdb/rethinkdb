//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/mpl/print.hpp>

#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <boost/assign/std/vector.hpp>

#include <string>
#include <vector>
#include <iostream>

#include "test.hpp"

using namespace spirit_test;
using boost::spirit::unused_type;

///////////////////////////////////////////////////////////////////////////////
struct action 
{
    action (std::vector<char>& vec) 
      : vec(vec), it(vec.begin()) 
    {}

    void operator()(unsigned& value, unused_type const&, bool& pass) const
    {
       pass = (it != vec.end());
       if (pass)
           value = *it++;
    }

    std::vector<char>& vec;
    mutable std::vector<char>::iterator it;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    using namespace boost::assign;

    std::vector<char> v;
    v += 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h';

    {
        BOOST_TEST(test("a,b,c,d,e,f,g,h", char_ % ',', v));
        BOOST_TEST(test_delimited("a , b , c , d , e , f , g , h ", 
            char_ % ',', v, space));
    }

    {
        std::string s ("abcdefgh");
        BOOST_TEST(test("a,b,c,d,e,f,g,h", char_ % ',', s));
        BOOST_TEST(test_delimited("a , b , c , d , e , f , g , h ", 
            char_ % ',', s, space));
    }

    {
        std::string s ("abcdefg");
        BOOST_TEST(test("abc,de,fg", char_ << ((char_ << char_) % ','), s));
        BOOST_TEST(test_delimited("a b c , d e , f g ", 
            char_ << ((char_ << char_) % ','), s, space));
    }

    { // actions
        namespace phx = boost::phoenix;

        BOOST_TEST(test("a,b,c,d,e,f,g,h", (char_ % ',')[_1 = phx::ref(v)]));
        BOOST_TEST(test_delimited("a , b , c , d , e , f , g , h ", 
            (char_ % ',')[_1 = phx::ref(v)], space));
    }

    // failing sub-generators
    {
        using boost::spirit::karma::strict;
        using boost::spirit::karma::relaxed;

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
        BOOST_TEST(test("d", r % ',', v2));
        BOOST_TEST(test("d", relaxed[r % ','], v2));
        BOOST_TEST(!test("", strict[r % ','], v2));

        r = &char_('a') << char_;
        BOOST_TEST(test("a", r % ',', v2));
        BOOST_TEST(test("a", relaxed[r % ','], v2));
        BOOST_TEST(test("a", strict[r % ','], v2));

        r = &char_('g') << char_;
        BOOST_TEST(test("g", r % ',', v2));
        BOOST_TEST(test("g", relaxed[r % ','], v2));
        BOOST_TEST(!test("", strict[r % ','], v2));

        r = !char_('d') << char_;
        BOOST_TEST(test("a,b,c,e,f,g", r % ',', v2));
        BOOST_TEST(test("a,b,c,e,f,g", relaxed[r % ','], v2));
        BOOST_TEST(test("a,b,c", strict[r % ','], v2));

        r = !char_('a') << char_;
        BOOST_TEST(test("b,c,d,e,f,g", r % ',', v2));
        BOOST_TEST(test("b,c,d,e,f,g", relaxed[r % ','], v2));
        BOOST_TEST(!test("", strict[r % ','], v2));

        r = !char_('g') << char_;
        BOOST_TEST(test("a,b,c,d,e,f", r % ',', v2));
        BOOST_TEST(test("a,b,c,d,e,f", relaxed[r % ','], v2));
        BOOST_TEST(test("a,b,c,d,e,f", strict[r % ','], v2));

        r = &char_('A') << char_;
        BOOST_TEST(!test("", r % ',', v2));
    }

    {
        // make sure user defined end condition is applied if no attribute
        // is passed in
        BOOST_TEST(test("[61,62,63,64,65,66,67,68]", 
            '[' << hex[action(v)] % ',' << ']'));
    }

    return boost::report_errors();
}

