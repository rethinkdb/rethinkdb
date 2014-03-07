/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <string>
#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace boost::spirit::ascii;
    using boost::spirit::qi::lit;
    using boost::spirit::qi::unused;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::double_;
    using boost::spirit::qi::what;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::_2;

    using boost::fusion::vector;
    using boost::fusion::at_c;

    using spirit_test::test;
    using spirit_test::test_attr;
    using spirit_test::print_info;

    {
        BOOST_TEST((test("aa", char_ >> char_)));
        BOOST_TEST((test("aaa", char_ >> char_ >> char_('a'))));
        BOOST_TEST((test("xi", char_('x') >> char_('i'))));
        BOOST_TEST((!test("xi", char_('x') >> char_('o'))));
        BOOST_TEST((test("xin", char_('x') >> char_('i') >> char_('n'))));
    }

    {
        BOOST_TEST((test(" a a", char_ >> char_, space)));
        BOOST_TEST((test(" x i", char_('x') >> char_('i'), space)));
        BOOST_TEST((!test(" x i", char_('x') >> char_('o'), space)));
    }

    {
        BOOST_TEST((test(" Hello, World", lit("Hello") >> ',' >> "World", space)));
    }

    {
        vector<char, char> attr;
        BOOST_TEST((test_attr("abcdefg", char_ >> char_ >> "cdefg", attr)));
        BOOST_TEST((at_c<0>(attr) == 'a'));
        BOOST_TEST((at_c<1>(attr) == 'b'));
    }

    {
        vector<char, char, char> attr;
        BOOST_TEST((test_attr(" a\n  b\n  c", char_ >> char_ >> char_, attr, space)));
        BOOST_TEST((at_c<0>(attr) == 'a'));
        BOOST_TEST((at_c<1>(attr) == 'b'));
        BOOST_TEST((at_c<2>(attr) == 'c'));
    }

    {
        // unused_type means we don't care about the attribute
        vector<char, char> attr;
        BOOST_TEST((test_attr("abc", char_ >> 'b' >> char_, attr)));
        BOOST_TEST((at_c<0>(attr) == 'a'));
        BOOST_TEST((at_c<1>(attr) == 'c'));
    }

    {
        // unused_type means we don't care about the attribute, even at the end
        vector<char, char> attr;
        BOOST_TEST((test_attr("acb", char_ >> char_ >> 'b', attr)));
        BOOST_TEST((at_c<0>(attr) == 'a'));
        BOOST_TEST((at_c<1>(attr) == 'c'));
    }

    {
        // "hello" has an unused_type. unused attributes are not part of the sequence
        vector<char, char> attr;
        BOOST_TEST((test_attr("a hello c", char_ >> "hello" >> char_, attr, space)));
        BOOST_TEST((at_c<0>(attr) == 'a'));
        BOOST_TEST((at_c<1>(attr) == 'c'));
    }

    {
        // a single element
        char attr;
        BOOST_TEST((test_attr("ab", char_ >> 'b', attr)));
        BOOST_TEST((attr == 'a'));
    }

    {
        // a single element fusion sequence
        vector<char> attr;
        BOOST_TEST((test_attr("ab", char_ >> 'b', attr)));
        BOOST_TEST((at_c<0>(attr) == 'a'));
    }

    {
        // make sure single element tuples get passed through if the rhs
        // has a single element tuple as its attribute
        vector<double, int> fv;
        rule<char const*, vector<double, int>()> r;
        r %= double_ >> ',' >> int_;
        BOOST_TEST((test_attr("test:2.0,1", "test:" >> r, fv) &&
            fv == vector<double, int>(2.0, 1)));
    }

    {
        // unused means we don't care about the attribute
        BOOST_TEST((test_attr("abc", char_ >> 'b' >> char_, unused)));
    }

    {
        BOOST_TEST((test("aA", no_case[char_('a') >> 'a'])));
        BOOST_TEST((test("BEGIN END", no_case[lit("begin") >> "end"], space)));
        BOOST_TEST((!test("BEGIN END", no_case[lit("begin") >> "nend"], space)));
    }

    {
#ifdef SPIRIT_NO_COMPILE_CHECK
        char_ >> char_ = char_ >> char_; // disallow this!
#endif
    }

    { // alternative forms of attributes. Allow sequences to take in
      // stl containers.

        std::vector<char> v;
        BOOST_TEST(test_attr("abc", char_ >> char_ >> char_, v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');
    }

    { // alternative forms of attributes. Allow sequences to take in
      // stl containers.

        std::vector<char> v;
        BOOST_TEST(test_attr("a,b,c", char_ >> *(',' >> char_), v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');
    }

    { // alternative forms of attributes. Allow sequences to take in
      // stl containers.

        std::vector<char> v;
        BOOST_TEST(test_attr("abc", char_ >> *char_, v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');
    }

    { // alternative forms of attributes. Allow sequences to take in
      // stl containers.
        using boost::spirit::qi::hold;

        std::vector<char> v;
        BOOST_TEST(test_attr("abc", char_ >> *(char_ >> char_), v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');

        v.clear();
        BOOST_TEST(!test_attr("abcd", char_ >> *(char_ >> char_), v));

        v.clear();
        BOOST_TEST(test_attr("abcdef", char_ >> *hold[char_ >> char_] >> char_, v));
        BOOST_TEST(v.size() == 6);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');
        BOOST_TEST(v[3] == 'd');
        BOOST_TEST(v[4] == 'e');
        BOOST_TEST(v[5] == 'f');

        v.clear();
        BOOST_TEST(test_attr("abc", char_ >> +(char_ >> char_), v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');
    }

    { // alternative forms of attributes. Allow sequences to take in
      // stl containers.

        std::vector<char> v;
        BOOST_TEST(test_attr("abc", char_ >> -(+char_), v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == 'a');
        BOOST_TEST(v[1] == 'b');
        BOOST_TEST(v[2] == 'c');
    }

    { // alternative forms of attributes. Allow sequences to take in
      // stl containers.

        std::string s;
        BOOST_TEST(test_attr("foobar", string("foo") >> string("bar"), s));
        BOOST_TEST(s == "foobar");

        s.clear();

        using boost::spirit::qi::hold;

        rule<char const*, std::string()> word = +char_("abc");
        BOOST_TEST(test_attr("ab.bc.ca", *hold[word >> string(".")] >> word, s));
        BOOST_TEST(s == "ab.bc.ca");
    }

    // alternative forms of attributes. Allow sequences to take in
    // stl containers of stl containers.
    {
        std::vector<std::string> v;
        BOOST_TEST(test_attr("abc1,abc2",
            *~char_(',') >> *(',' >> *~char_(',')), v));
        BOOST_TEST(v.size() == 2 && v[0] == "abc1" && v[1] == "abc2");
    }

    {
        std::vector<std::string> v;
        rule<char const*, std::string()> e = *~char_(',');
        rule<char const*, std::vector<std::string>()> l = e >> *(',' >> e);

        BOOST_TEST(test_attr("abc1,abc2,abc3", l, v));
        BOOST_TEST(v.size() == 3);
        BOOST_TEST(v[0] == "abc1");
        BOOST_TEST(v[1] == "abc2");
        BOOST_TEST(v[2] == "abc3");
    }

    // do the same with a plain string object
    {
        std::string s;
        BOOST_TEST(test_attr("abc1,abc2",
            *~char_(',') >> *(',' >> *~char_(',')), s));
        BOOST_TEST(s == "abc1abc2");
    }

    {
        std::string s;
        rule<char const*, std::string()> e = *~char_(',');
        rule<char const*, std::string()> l = e >> *(',' >> e);

        BOOST_TEST(test_attr("abc1,abc2,abc3", l, s));
        BOOST_TEST(s == "abc1abc2abc3");
    }

    {
        std::vector<char> v;
        BOOST_TEST(test_attr("ab", char_ >> -char_, v));
        BOOST_TEST(v.size() == 2 && v[0] == 'a' && v[1] == 'b');

        v.clear();
        BOOST_TEST(test_attr("a", char_ >> -char_, v));
        BOOST_TEST(v.size() == 1 && v[0] == 'a');

        v.clear();
        BOOST_TEST(test_attr("a", char_, v));
        BOOST_TEST(v.size() == 1 && v[0] == 'a');
    }

    {
        std::vector<boost::optional<char> > v;
        BOOST_TEST(test_attr("ab", char_ >> -char_, v));
        BOOST_TEST(v.size() == 2 && v[0] == 'a' && v[1] == 'b');

        v.clear();
        BOOST_TEST(test_attr("a", char_ >> -char_, v));
        BOOST_TEST(v.size() == 2 && v[0] == 'a' && !v[1]);

        v.clear();
        BOOST_TEST(test_attr("a", char_, v));
        BOOST_TEST(v.size() == 1 && v[0] == 'a');
    }

    {   // test action
        using boost::phoenix::ref;
        char c = 0;
        int n = 0;

        BOOST_TEST(test("x123\"a string\"", (char_ >> int_ >> "\"a string\"")
            [ref(c) = _1, ref(n) = _2]));
        BOOST_TEST(c == 'x');
        BOOST_TEST(n == 123);
    }

    {   // test action
        using boost::phoenix::ref;
        char c = 0;
        int n = 0;

        BOOST_TEST(test("x 123 \"a string\"", (char_ >> int_ >> "\"a string\"")
            [ref(c) = _1, ref(n) = _2], space));
        BOOST_TEST(c == 'x');
        BOOST_TEST(n == 123);
    }

    {   // testing "what"
        print_info(what(alpha | char_('x') >> lit("hello") >> int_));
    }

//     { // compile check only
//         using boost::spirit::qi::rule;
//         typedef boost::fusion::vector<int, double> tuple_type;
//         typedef std::vector<boost::fusion::vector<int, double> > attr_type;
// 
//         rule<char const*, tuple_type()> r = int_ >> ',' >> double_;
//         rule<char const*, attr_type()> r2 = r >> *(',' >> r);
//         //~ rule<char const*, attr_type()> r2 = r % ',';
//     }

    return boost::report_errors();
}

