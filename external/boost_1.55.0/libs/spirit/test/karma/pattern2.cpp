//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost;
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    typedef spirit_test::output_iterator<char>::type outiter_type;

    // locals test
    {
        karma::rule<outiter_type, locals<std::string> > start;

        start = string[_1 = "abc", _a = _1] << int_[_1 = 10] << string[_1 = _a];
        BOOST_TEST(test("abc10abc", start));
    }

    {
        karma::rule<outiter_type, space_type, locals<std::string> > start;

        start = string[_1 = "abc", _a = _1] << int_[_1 = 10] << string[_1 = _a];
        BOOST_TEST(test_delimited("abc 10 abc ", start, space));
    }

    // alias tests
    { 
        typedef variant<char, int, double> var_type;

        karma::rule<outiter_type, var_type()> d, start;

        d = start.alias();   // d will always track start

        start = (char_ | int_ | double_)[_1 = _val];

        var_type v ('a');
        BOOST_TEST(test("a", d, v));
        v = 10;
        BOOST_TEST(test("10", d, v));
        v = 12.4;
        BOOST_TEST(test("12.4", d, v));
    }

    { 
        typedef variant<char, int, double> var_type;

        karma::rule<outiter_type, space_type, var_type()> d, start;

        d = start.alias();   // d will always track start

        start = (char_ | int_ | double_)[_1 = _val];

        var_type v ('a');
        BOOST_TEST(test_delimited("a ", d, v, space));
        v = 10;
        BOOST_TEST(test_delimited("10 ", d, v, space));
        v = 12.4;
        BOOST_TEST(test_delimited("12.4 ", d, v, space));
    }

    {
        typedef variant<char, int, double> var_type;

        karma::rule<outiter_type, var_type()> d, start;

        d = start.alias();   // d will always track start

        start %= char_ | int_ | double_;

        var_type v ('a');
        BOOST_TEST(test("a", d, v));
        v = 10;
        BOOST_TEST(test("10", d, v));
        v = 12.4;
        BOOST_TEST(test("12.4", d, v));

        start = char_ | int_ | double_;

        v = 'a';
        BOOST_TEST(test("a", d, v));
        v = 10;
        BOOST_TEST(test("10", d, v));
        v = 12.4;
        BOOST_TEST(test("12.4", d, v));
    }

    {
        typedef variant<char, int, double> var_type;

        karma::rule<outiter_type, space_type, var_type()> d, start;

        d = start.alias();   // d will always track start

        start %= char_ | int_ | double_;

        var_type v ('a');
        BOOST_TEST(test_delimited("a ", d, v, space));
        v = 10;
        BOOST_TEST(test_delimited("10 ", d, v, space));
        v = 12.4;
        BOOST_TEST(test_delimited("12.4 ", d, v, space));

        start = char_ | int_ | double_;

        v = 'a';
        BOOST_TEST(test_delimited("a ", d, v, space));
        v = 10;
        BOOST_TEST(test_delimited("10 ", d, v, space));
        v = 12.4;
        BOOST_TEST(test_delimited("12.4 ", d, v, space));
    }

    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::karma::int_;
        using boost::spirit::karma::_1;
        using boost::spirit::karma::_val;
        using boost::spirit::karma::space;
        using boost::spirit::karma::space_type;

        karma::rule<outiter_type, int()> r1 = int_;
        karma::rule<outiter_type, space_type, int()> r2 = int_;

        int i = 123;
        int j = 456;
        BOOST_TEST(test("123", r1[_1 = _val], i));
        BOOST_TEST(test_delimited("456 ", r2[_1 = _val], j, space));
    }

    return boost::report_errors();
}

