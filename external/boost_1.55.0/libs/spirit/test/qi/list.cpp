/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <string>
#include <vector>
#include <set>
#include <map>

#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <string>
#include <iostream>
#include "test.hpp"

using namespace spirit_test;

int
main()
{
    using namespace boost::spirit::ascii;

    {
        BOOST_TEST(test("a,b,c,d,e,f,g,h", char_ % ','));
        BOOST_TEST(test("a,b,c,d,e,f,g,h,", char_ % ',', false));
    }

    {
        BOOST_TEST(test("a, b, c, d, e, f, g, h", char_ % ',', space));
        BOOST_TEST(test("a, b, c, d, e, f, g, h,", char_ % ',', space, false));
    }

    {
        std::string s;
        BOOST_TEST(test_attr("a,b,c,d,e,f,g,h", char_ % ',', s));
        BOOST_TEST(s == "abcdefgh");

        BOOST_TEST(!test("a,b,c,d,e,f,g,h,", char_ % ','));
    }

    {
        std::string s;
        BOOST_TEST(test_attr("ab,cd,ef,gh", (char_ >> char_) % ',', s));
        BOOST_TEST(s == "abcdefgh");

        BOOST_TEST(!test("ab,cd,ef,gh,", (char_ >> char_) % ','));
        BOOST_TEST(!test("ab,cd,ef,g", (char_ >> char_) % ','));

        s.clear();
        BOOST_TEST(test_attr("ab,cd,efg", (char_ >> char_) % ',' >> char_, s));
        BOOST_TEST(s == "abcdefg");
    }

    {
        using boost::spirit::int_;

        std::vector<int> v;
        BOOST_TEST(test_attr("1,2", int_ % ',', v));
        BOOST_TEST(2 == v.size() && 1 == v[0] && 2 == v[1]);
    }

    {
        using boost::spirit::int_;

        std::vector<int> v;
        BOOST_TEST(test_attr("(1,2)", '(' >> int_ % ',' >> ')', v));
        BOOST_TEST(2 == v.size() && 1 == v[0] && 2 == v[1]);
    }

    {
        std::vector<std::string> v;
        BOOST_TEST(test_attr("a,b,c,d", +alpha % ',', v));
        BOOST_TEST(4 == v.size() && "a" == v[0] && "b" == v[1]
            && "c" == v[2] && "d" == v[3]);
    }

    {
        std::vector<boost::optional<char> > v;
        BOOST_TEST(test_attr("#a,#", ('#' >> -alpha) % ',', v)); 
        BOOST_TEST(2 == v.size() && 
            !!v[0] && 'a' == boost::get<char>(v[0]) && !v[1]);

        std::vector<char> v2;
        BOOST_TEST(test_attr("#a,#", ('#' >> -alpha) % ',', v2)); 
        BOOST_TEST(1 == v2.size() && 'a' == v2[0]);
    }

    {
        typedef std::set<std::pair<std::string, std::string> > set_type;
        set_type s;
        BOOST_TEST(test_attr("k1=v1&k2=v2", 
            (*(char_ - '=') >> '=' >> *(char_ - '&')) % '&', s));

        set_type::const_iterator it = s.begin();
        BOOST_TEST(s.size() == 2);
        BOOST_TEST(it != s.end() && (*it).first == "k1" && (*it).second == "v1");
        BOOST_TEST(++it != s.end() && (*it).first == "k2" && (*it).second == "v2");
    }

    {
        typedef std::map<std::string, std::string> map_type;
        map_type m;
        BOOST_TEST(test_attr("k1=v1&k2=v2", 
            (*(char_ - '=') >> '=' >> *(char_ - '&')) % '&', m));

        map_type::const_iterator it = m.begin();
        BOOST_TEST(m.size() == 2);
        BOOST_TEST(it != m.end() && (*it).first == "k1" && (*it).second == "v1");
        BOOST_TEST(++it != m.end() && (*it).first == "k2" && (*it).second == "v2");
    }

    { // actions
        namespace phx = boost::phoenix;
        using boost::phoenix::begin;
        using boost::phoenix::end;
        using boost::phoenix::construct;
        using boost::spirit::qi::_1;

        std::string s;
        BOOST_TEST(test("a,b,c,d,e,f,g,h", (char_ % ',')
            [phx::ref(s) = construct<std::string>(begin(_1), end(_1))]));
    }

    return boost::report_errors();
}

