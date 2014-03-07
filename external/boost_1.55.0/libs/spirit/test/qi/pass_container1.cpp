//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
#include <boost/spirit/include/qi_auxiliary.hpp>
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

inline bool compare(std::vector<char> const& v, std::string const& s)
{
    return v.size() == s.size() && std::equal(v.begin(), v.end(), s.begin());
}

int main()
{
    using boost::spirit::qi::char_;
    using boost::spirit::qi::omit;

    {
        std::vector<char> v;
        BOOST_TEST(test_attr("a,b,c,d,e,f,g,h", char_ % ',', v) &&
            compare(v, "abcdefgh"));

        std::string s;
        BOOST_TEST(test_attr("a,b,c,d,e,f,g,h", char_ % ',', s) && 
            s == "abcdefgh");

        BOOST_TEST(test("a,b,c,d,e,f,g,h", char_ % ','));
        BOOST_TEST(test("a,b,c,d,e,f,g,h", omit[char_] % ','));
    }

    {
        std::vector<char> v1;
        BOOST_TEST(test_attr("ab,cd,ef,gh", (char_ >> char_) % ',', v1) &&
            compare(v1, "abcdefgh"));
        v1.clear();
        BOOST_TEST(test_attr("ab,cd,ef,gh", (char_ >> omit[char_]) % ',', v1) &&
            compare(v1, "aceg"));

        std::string s;
        BOOST_TEST(test_attr("ab,cd,ef,gh", (char_ >> char_) % ',', s) && 
            s == "abcdefgh");
        s.clear();
        BOOST_TEST(test_attr("ab,cd,ef,gh", (char_ >> omit[char_]) % ',', s) && 
            s == "aceg");

        std::vector<std::pair<char, char> > v2;
        BOOST_TEST(test_attr("ab,cd,ef,gh", (char_ >> char_) % ',', v2) &&
            v2.size() == 4 && 
            v2[0] == std::make_pair('a', 'b') && 
            v2[1] == std::make_pair('c', 'd') &&
            v2[2] == std::make_pair('e', 'f') &&
            v2[3] == std::make_pair('g', 'h'));

        s.clear();
        BOOST_TEST(test_attr("ab,cd,efg", (char_ >> char_) % ',' >> char_, s) &&
            s == "abcdefg");

        BOOST_TEST(test("ab,cd,ef,gh", (char_ >> char_) % ','));
        BOOST_TEST(test("ab,cd,ef,gh", (omit[char_ >> char_]) % ','));
    }

    {
        std::vector<char> v1;
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> *~char_(',')) % ',', v1) &&
            compare(v1, "abcdefgh"));
        v1.clear();
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> omit[*~char_(',')]) % ',', v1) &&
            compare(v1, "adg"));
        v1.clear();
        BOOST_TEST(test_attr("abc,def,gh", (omit[char_] >> *~char_(',')) % ',', v1) &&
            compare(v1, "bcefh"));

        std::string s1;
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> *~char_(',')) % ',', s1) && 
            s1 == "abcdefgh");
        s1.clear();
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> omit[*~char_(',')]) % ',', s1) && 
            s1 == "adg");
        s1.clear();
        BOOST_TEST(test_attr("abc,def,gh", (omit[char_] >> *~char_(',')) % ',', s1) && 
            s1 == "bcefh");

        std::vector<std::pair<char, std::vector<char> > > v2;
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> *~char_(',')) % ',', v2) &&
            v2.size() == 3 && 
            v2[0].first == 'a' && compare(v2[0].second, "bc") && 
            v2[1].first == 'd' && compare(v2[1].second, "ef") &&
            v2[2].first == 'g' && compare(v2[2].second, "h"));

        std::vector<std::vector<char> > v3;
        BOOST_TEST(test_attr("abc,def,gh", (omit[char_] >> *~char_(',')) % ',', v3) &&
            v3.size() == 3 && 
            compare(v3[0], "bc") && compare(v3[1], "ef") &&
            compare(v3[2], "h"));

        std::vector<char> v4;
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> omit[*~char_(',')]) % ',', v4) &&
            v4.size() == 3 && 
            v4[0] == 'a' &&  v4[1] == 'd' && v4[2] == 'g');

        std::vector<std::string> v5;
        BOOST_TEST(test_attr("abc,def,gh", (omit[char_] >> *~char_(',')) % ',', v5) &&
            v5.size() == 3 && 
            v5[0] == "bc" && v5[1] == "ef" && v5[2] == "h");

        std::string s2;
        BOOST_TEST(test_attr("abc,def,gh", (char_ >> omit[*~char_(',')]) % ',', s2) &&
            s2.size() == 3 && 
            s2 == "adg");

        BOOST_TEST(test("abc,def,gh", (char_ >> *~char_(',')) % ','));
        BOOST_TEST(test("abc,def,gh", (omit[char_ >> *~char_(',')]) % ','));
    }

    {
        using boost::spirit::qi::alpha;
        using boost::spirit::qi::digit;

        std::vector<char> v1;
        BOOST_TEST(test_attr("ab12,cd34,ef56", (*alpha >> *digit) % ',', v1) &&
            compare(v1, "ab12cd34ef56"));
        v1.clear();
        BOOST_TEST(test_attr("ab12,cd34,ef56", (omit[*alpha] >> *digit) % ',', v1) &&
            compare(v1, "123456"));
        v1.clear();
        BOOST_TEST(test_attr("ab12,cd34,ef56", (*alpha >> omit[*digit]) % ',', v1) &&
            compare(v1, "abcdef"));

        std::string s1;
        BOOST_TEST(test_attr("ab12,cd34,ef56", (*alpha >> *digit) % ',', s1) &&
            s1 == "ab12cd34ef56");
        s1.clear();
        BOOST_TEST(test_attr("ab12,cd34,ef56", (omit[*alpha] >> *digit) % ',', s1) &&
            s1 == "123456");
        s1.clear();
        BOOST_TEST(test_attr("ab12,cd34,ef56", (*alpha >> omit[*digit]) % ',', s1) &&
            s1 == "abcdef");

        std::vector<std::pair<std::vector<char>, std::vector<char> > > v2;
        BOOST_TEST(test_attr("ab12,cd34,ef56", (*alpha >> *digit) % ',', v2) &&
            v2.size() == 3 && 
            compare(v2[0].first, "ab") && compare(v2[0].second, "12") &&
            compare(v2[1].first, "cd") && compare(v2[1].second, "34") &&
            compare(v2[2].first, "ef") && compare(v2[2].second, "56"));

        std::vector<std::pair<std::string, std::string> > v3;
        BOOST_TEST(test_attr("ab12,cd34,ef56", (*alpha >> *digit) % ',', v3) &&
            v3.size() == 3 && 
            v3[0].first == "ab" && v3[0].second == "12" &&
            v3[1].first == "cd" && v3[1].second == "34" &&
            v3[2].first == "ef" && v3[2].second == "56");

        std::vector<std::vector<char> > v4;
        BOOST_TEST(test_attr("ab12,cd34,ef56", (omit[*alpha] >> *digit) % ',', v4) &&
            v4.size() == 3 && 
            compare(v4[0], "12") &&
            compare(v4[1], "34") &&
            compare(v4[2], "56"));

        BOOST_TEST(test("ab12,cd34,ef56", (*alpha >> *digit) % ','));
        BOOST_TEST(test("ab12,cd34,ef56", omit[*alpha >> *digit] % ','));
    }

    return boost::report_errors();
}

