//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2009 Francois Barel
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

#include <boost/spirit/repository/include/karma_subrule.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost;
    using namespace boost::spirit;
    using namespace boost::spirit::karma;
//    using namespace boost::spirit::ascii;
    using boost::spirit::repository::karma::subrule;

    typedef spirit_test::output_iterator<char>::type outiter_type;

    // basic tests
    {
        rule<outiter_type> start;
        subrule<0> sr;

        start = (
            sr = char_[_1 = 'a'] << int_[_1 = 10] << double_[_1 = 12.4]
        );
        BOOST_TEST(test("a1012.4", start));

        BOOST_TEST(test("a1012.4", (
            sr = (char_ << int_ << double_)[_1 = 'a', _2 = 10, _3 = 12.4]
        )));

        subrule<1> a;
        subrule<2> b;
        subrule<3> c;

        start = (
            sr = a << b << c
          , a = char_[_1 = 'a']
          , b = int_[_1 = 10]
          , c = double_[_1 = 12.4]
        );
        BOOST_TEST(test("a1012.4", start));
    }

    // basic tests with delimiter
    {
        rule<outiter_type, space_type> start;
        subrule<0> sr;

        start = (
            sr = char_[_1 = 'a'] << int_[_1 = 10] << double_[_1 = 12.4]
        );
        BOOST_TEST(test_delimited("a 10 12.4 ", start, space));

        BOOST_TEST(test_delimited("a 10 12.4 ", (
            sr = (char_ << int_ << double_)[_1 = 'a', _2 = 10, _3 = 12.4]
        ), space));

        subrule<1> a;
        subrule<2> b;
        subrule<3> c;

        start = (
            sr = a << b << c
          , a = char_[_1 = 'a']
          , b = int_[_1 = 10]
          , c = double_[_1 = 12.4]
        );
        BOOST_TEST(test_delimited("a 10 12.4 ", start, space));
    }

    // basic tests involving a direct parameter
    {
        typedef variant<char, int, double> var_type;

        rule<outiter_type, var_type()> start;
        subrule<0, var_type()> sr;

        start = (
            sr = (char_ | int_ | double_)[_1 = _r0]
        )[_1 = _val];

        var_type v ('a');
        BOOST_TEST(test("a", start, v));
        v = 10;
        BOOST_TEST(test("10", start, v));
        v = 12.4;
        BOOST_TEST(test("12.4", start, v));
    }

    {
        typedef variant<char, int, double> var_type;

        rule<outiter_type, space_type, var_type()> start;
        subrule<0, var_type()> sr;

        start %= (
            sr = (char_ | int_ | double_)[_1 = _r0]
        );

        var_type v ('a');
        BOOST_TEST(test_delimited("a ", start, v, space));
        v = 10;
        BOOST_TEST(test_delimited("10 ", start, v, space));
        v = 12.4;
        BOOST_TEST(test_delimited("12.4 ", start, v, space));
    }

    {
        rule<outiter_type, void(char, int, double)> start;
        subrule<0, void(char, int, double)> sr;

        start = (
            sr = char_[_1 = _r1] << int_[_1 = _r2] << double_[_1 = _r3]
        )(_r1, _r2, _r3);
        BOOST_TEST(test("a1012.4", start('a', 10, 12.4)));

        BOOST_TEST(test("a1012.4", (
            sr = (char_ << int_ << double_)[_1 = _r1, _2 = _r2, _3 = _r3]
        )('a', 10, 12.4)));

        subrule<1, void(char, int, double)> entry;
        subrule<2, void(char)> a;
        subrule<3, void(int)> b;
        subrule<4, void(double)> c;

        start = (
            entry = a(_r1) << b(_r2) << c(_r3)
          , a = char_[_1 = _r1]
          , b = int_[_1 = _r1]
          , c = double_[_1 = _r1]
        )(_r1, _r2, _r3);
        BOOST_TEST(test("a1012.4", start('a', 10, 12.4)));
    }

    {
        rule<outiter_type, space_type, void(char, int, double)> start;
        subrule<0, void(char, int, double)> sr;

        start = (
            sr = char_[_1 = _r1] << int_[_1 = _r2] << double_[_1 = _r3]
        )(_r1, _r2, _r3);
        BOOST_TEST(test_delimited("a 10 12.4 ", start('a', 10, 12.4), space));

        BOOST_TEST(test_delimited("a 10 12.4 ", (
            sr = (char_ << int_ << double_)[_1 = _r1, _2 = _r2, _3 = _r3]
        )('a', 10, 12.4), space));

        subrule<1, void(char, int, double)> entry;
        subrule<2, void(char)> a;
        subrule<3, void(int)> b;
        subrule<4, void(double)> c;

        start = (
            entry = a(_r1) << b(_r2) << c(_r3)
          , a = char_[_1 = _r1]
          , b = int_[_1 = _r1]
          , c = double_[_1 = _r1]
        )(_r1, _r2, _r3);
        BOOST_TEST(test_delimited("a 10 12.4 ", start('a', 10, 12.4), space));
    }

    return boost::report_errors();
}

