//  Copyright (c) 2001-2010 Hartmut Kaiser
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

    // test rule parameter propagation
    {
        using boost::phoenix::at_c;

        karma::rule<outiter_type, fusion::vector<char, int, double>()> start;
        fusion::vector<char, int, double> vec('a', 10, 12.4);

        start %= char_ << int_ << double_;
        BOOST_TEST(test("a1012.4", start, vec));

        karma::rule<outiter_type, char()> a;
        karma::rule<outiter_type, int()> b;
        karma::rule<outiter_type, double()> c;

        a %= char_ << eps;
        b %= int_;
        c %= double_;
        start = a[_1 = at_c<0>(_r0)] << b[_1 = at_c<1>(_r0)] << c[_1 = at_c<2>(_r0)];
        BOOST_TEST(test("a1012.4", start, vec));

        start = (a << b << c)[_1 = at_c<0>(_r0), _2 = at_c<1>(_r0), _3 = at_c<2>(_r0)];
        BOOST_TEST(test("a1012.4", start, vec));

        start = a << b << c;
        BOOST_TEST(test("a1012.4", start, vec));

        start %= a << b << c;
        BOOST_TEST(test("a1012.4", start, vec));
    }

    {
        using boost::phoenix::at_c;

        karma::rule<outiter_type, space_type, fusion::vector<char, int, double>()> start;
        fusion::vector<char, int, double> vec('a', 10, 12.4);

        start %= char_ << int_ << double_;
        BOOST_TEST(test_delimited("a 10 12.4 ", start, vec, space));

        karma::rule<outiter_type, space_type, char()> a;
        karma::rule<outiter_type, space_type, int()> b;
        karma::rule<outiter_type, space_type, double()> c;

        a %= char_ << eps;
        b %= int_;
        c %= double_;
        start = a[_1 = at_c<0>(_r0)] << b[_1 = at_c<1>(_r0)] << c[_1 = at_c<2>(_r0)];
        BOOST_TEST(test_delimited("a  10 12.4 ", start, vec, space));

        start = (a << b << c)[_1 = at_c<0>(_r0), _2 = at_c<1>(_r0), _3 = at_c<2>(_r0)];
        BOOST_TEST(test_delimited("a  10 12.4 ", start, vec, space));

        start = a << b << c;
        BOOST_TEST(test_delimited("a  10 12.4 ", start, vec, space));

        start %= a << b << c;
        BOOST_TEST(test_delimited("a  10 12.4 ", start, vec, space));
    }

    // test direct initalization
    {
        using boost::phoenix::at_c;

        fusion::vector<char, int, double> vec('a', 10, 12.4);
        karma::rule<outiter_type, space_type, fusion::vector<char, int, double>()> 
            start = char_ << int_ << double_;;

        BOOST_TEST(test_delimited("a 10 12.4 ", start, vec, space));

        karma::rule<outiter_type, space_type, char()> a = char_ << eps;
        karma::rule<outiter_type, space_type, int()> b = int_;
        karma::rule<outiter_type, space_type, double()> c = double_;

        start = a[_1 = at_c<0>(_r0)] << b[_1 = at_c<1>(_r0)] << c[_1 = at_c<2>(_r0)];
        BOOST_TEST(test_delimited("a  10 12.4 ", start, vec, space));
    }

    return boost::report_errors();
}

