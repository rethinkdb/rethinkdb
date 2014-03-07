//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define KARMA_TEST_COMPILE_FAIL

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    {
        BOOST_TEST(test("x", char_('x') | char_('i')));
        BOOST_TEST(test("xi", char_('x') << char_('i') | char_('i')));
        BOOST_TEST(test("i", char_('i') | char_('x') << char_('i')));

        BOOST_TEST(test("x", buffer[char_('x')] | char_('i')));

        variant<int, char> v (10);
        BOOST_TEST(test("10", char_ | int_, v));
        BOOST_TEST(test("10", int_ | char_, v));
        BOOST_TEST(test("a", lit('a') | char_ | int_, v));
        BOOST_TEST(test("a", char_ | lit('a') | int_, v));
        BOOST_TEST(test("10", int_ | lit('a') | char_, v));

        v = 'c';
        BOOST_TEST(test("c", char_ | int_, v));
        BOOST_TEST(test("a", lit('a') | char_ | int_, v));
        BOOST_TEST(test("c", char_ | lit('a') | int_, v));
        BOOST_TEST(test("a", int_ | lit('a') | char_, v));
        BOOST_TEST(test("c", int_ | char_ | lit('a'), v));
    }

    // testing for alignment/truncation problems on little endian systems
    // (big endian systems will fail one of the other tests below)
    {
        // test optional attribute
        optional<variant<int, char> > v;
        BOOST_TEST(!test("", char_ | int_, v));
        BOOST_TEST(!test("", int_ | char_, v));
        BOOST_TEST(test("a", lit('a') | char_ | int_, v));
        BOOST_TEST(test("a", char_ | lit('a') | int_, v));
        BOOST_TEST(test("a", int_ | lit('a') | char_, v));

        v = 10;
        BOOST_TEST(test("10", char_ | int_, v));
        BOOST_TEST(test("10", int_ | char_, v));
        BOOST_TEST(test("a", lit('a') | char_ | int_, v));
        BOOST_TEST(test("a", char_ | lit('a') | int_, v));
        BOOST_TEST(test("10", int_ | lit('a') | char_, v));

        v = 'c';
        BOOST_TEST(test("c", char_ | int_, v));
        BOOST_TEST(test("a", lit('a') | char_ | int_, v));
        BOOST_TEST(test("c", char_ | lit('a') | int_, v));
        BOOST_TEST(test("a", int_ | lit('a') | char_, v));
        BOOST_TEST(test("c", int_ | char_ | lit('a'), v));
    }

    {
        // more tests for optional attribute
        optional<int> o;
        BOOST_TEST(test("a", lit('a') | int_, o));
        BOOST_TEST(test("a", int_ | lit('a'), o));

        o = 10;
        BOOST_TEST(test("a", lit('a') | int_, o));
        BOOST_TEST(test("10", int_ | lit('a'), o));
    }

    {
        int i = 10;
        BOOST_TEST(test("a", lit('a') | int_, i));
        BOOST_TEST(test("10", int_ | lit('a'), i));
    }

    {
        BOOST_TEST(test("abc", string | int_, std::string("abc")));
        BOOST_TEST(test("1234", string | int_, 1234));
        BOOST_TEST(test("abc", int_ | string, std::string("abc")));
        BOOST_TEST(test("1234", int_ | string, 1234));
    }

    {
        // testing for alignment/truncation problems on little endian systems
        // (big endian systems will fail one of the other tests below)
        std::basic_string<wchar_t> generated;
        std::back_insert_iterator<std::basic_string<wchar_t> > outit(generated);
        boost::variant<int, char> v(10);
        bool result = karma::generate_delimited(outit
          , karma::int_ | karma::char_, karma::char_(' '), v);
        BOOST_TEST(result && generated == L"10 ");
    }

    {
        boost::optional<int> v;
        BOOST_TEST(test("error", int_ | "error" << omit[-int_], v));
        BOOST_TEST(test("error", int_ | "error" << omit[int_], v));
        v = 1;
        BOOST_TEST(test("1", int_ | "error" << omit[-int_], v));
        BOOST_TEST(test("1", int_ | "error" << omit[int_], v));
    }

    {
        typedef spirit_test::output_iterator<char>::type outiter_type;
        namespace karma = boost::spirit::karma;

        karma::rule<outiter_type, int()> r = int_;
        std::vector<int> v;
        BOOST_TEST(test("", '>' << r % ',' | karma::eps, v));

        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);
        BOOST_TEST(test(">1,2,3,4", '>' << r % ',' | karma::eps, v));
    }

    {
        typedef spirit_test::output_iterator<char>::type outiter_type;
        namespace karma = boost::spirit::karma;

        karma::rule<outiter_type, boost::optional<int>()> r = int_;
        boost::optional<int> o;
        BOOST_TEST(test("error", r | "error", o));

        o = 10;
        BOOST_TEST(test("10", r | "error", o));
    }

    return boost::report_errors();
}

