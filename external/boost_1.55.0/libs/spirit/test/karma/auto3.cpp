//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "auto.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        using spirit_test::test;
        using spirit_test::test_delimited;

        // test primitive types
        BOOST_TEST(test_rule("true", true));
        BOOST_TEST(test_rule("1", 1));
        BOOST_TEST(test_rule("1.1", 1.1));
        BOOST_TEST(test_rule("test", std::string("test")));

        // test containers
        std::vector<int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(2);
        BOOST_TEST(test_rule("012", v));
        BOOST_TEST(test_rule_delimited("0,1,2,", v, ','));

        std::list<int> l;
        l.push_back(0);
        l.push_back(1);
        l.push_back(2);
        BOOST_TEST(test_rule("012", l));
        BOOST_TEST(test_rule_delimited("0,1,2,", l, ','));

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_rule("", o));
        o = 1;
        BOOST_TEST(test_rule("1", o));

        // test alternative
        boost::variant<int, double, float, std::string> vv;
        vv = 1;
        BOOST_TEST(test_rule("1", vv));
        vv = 1.0;
        BOOST_TEST(test_rule("1.0", vv));
        vv = 1.0f;
        BOOST_TEST(test_rule("1.0", vv));
        vv = "some string";
        BOOST_TEST(test_rule("some string", vv));

        // test fusion sequence
        std::pair<int, double> p (1, 2.0);
        BOOST_TEST(test_rule("12.0", p));
        BOOST_TEST(test_rule_delimited("1,2.0,", p, ','));
    }

    return boost::report_errors();
}
