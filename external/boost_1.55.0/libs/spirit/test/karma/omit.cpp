//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <iostream>
#include "test.hpp"

using namespace spirit_test;

int main()
{
    using boost::spirit::karma::_1;
    using boost::spirit::karma::_a;
    using boost::spirit::karma::double_;
    using boost::spirit::karma::int_;
    using boost::spirit::karma::omit;
    using boost::spirit::karma::skip;
    using boost::spirit::karma::rule;
    using boost::spirit::karma::locals;

    typedef spirit_test::output_iterator<char>::type outiter_type;
    typedef std::pair<std::vector<int>, int> attribute_type;

    rule<outiter_type, attribute_type(), locals<int> > r;

    attribute_type a;
    a.first.push_back(0x01);
    a.first.push_back(0x02);
    a.first.push_back(0x04);
    a.first.push_back(0x08);
    a.second = 0;

    // omit[] is supposed to execute the embedded generator
    {
        std::pair<double, double> p (1.0, 2.0);
        BOOST_TEST(test("2.0", omit[double_] << double_, p));

        r %= omit[ *(int_[_a = _a + _1]) ] << int_[_1 = _a];
        BOOST_TEST(test("15", r, a));
    }

    // even if omit[] never fails, it has to honor the result of the 
    // embedded generator
    {
        typedef std::pair<double, double> attribute_type;
        rule<outiter_type, attribute_type()> r;

        r %= omit[double_(1.0) << double_] | "42";

        attribute_type p1 (1.0, 2.0);
        BOOST_TEST(test("", r, p1));

        attribute_type p2 (10.0, 2.0);
        BOOST_TEST(test("42", r, p2));
     }

    // skip[] is not supposed to execute the embedded generator
    {
        using boost::spirit::karma::double_;
        using boost::spirit::karma::skip;

        std::pair<double, double> p (1.0, 2.0);
        BOOST_TEST(test("2.0", skip[double_] << double_, p));

        r %= skip[ *(int_[_a = _a + _1]) ] << int_[_1 = _a];
        BOOST_TEST(test("0", r, a));
    }
    
    return boost::report_errors();
}
