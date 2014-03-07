//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "auto.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        using karma::auto_;
        using karma::upper;
        using spirit_test::test;
        using spirit_test::test_delimited;

        // test primitive types
        BOOST_TEST(test("true", auto_, true));
        BOOST_TEST(test("1", auto_, 1));
        BOOST_TEST(test("1.1", auto_, 1.1));
        BOOST_TEST(test("test", auto_, "test"));
        BOOST_TEST(test(L"test", auto_, L"test"));
        BOOST_TEST(test("a", auto_, 'a'));
        BOOST_TEST(test(L"a", auto_, L'a'));

        BOOST_TEST(test("TRUE", upper[auto_], true));
        BOOST_TEST(test("TEST", upper[auto_], "test"));

        // test containers
        std::vector<int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(2);
        BOOST_TEST(test("012", auto_, v));
        BOOST_TEST(test("0,1,2", auto_ % ',', v));
        BOOST_TEST(test_delimited("0,1,2,", auto_, v, ','));

        std::list<int> l;
        l.push_back(0);
        l.push_back(1);
        l.push_back(2);
        BOOST_TEST(test("012", auto_, l));
        BOOST_TEST(test("0,1,2", auto_ % ',', l));
        BOOST_TEST(test_delimited("0,1,2,", auto_, l, ','));

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test("", auto_, o));
        o = 1;
        BOOST_TEST(test("1", auto_, o));

        // test alternative
        boost::variant<int, double, float, std::string> vv;
        vv = 1;
        BOOST_TEST(test("1", auto_, vv));
        vv = 1.0;
        BOOST_TEST(test("1.0", auto_, vv));
        vv = 1.0f;
        BOOST_TEST(test("1.0", auto_, vv));
        vv = "some string";
        BOOST_TEST(test("some string", auto_, vv));

        // test fusion sequence
        std::pair<int, double> p (1, 2.0);
        BOOST_TEST(test("12.0", auto_, p));
        BOOST_TEST(test_delimited("1,2.0,", auto_, p, ','));
    }

    {
        using karma::auto_;
        using karma::upper;
        using spirit_test::test;
        using spirit_test::test_delimited;

        // test primitive types
        BOOST_TEST(test("true", auto_(true)));
        BOOST_TEST(test("1", auto_(1)));
        BOOST_TEST(test("1.1", auto_(1.1)));
        BOOST_TEST(test("test", auto_("test")));
        BOOST_TEST(test(L"test", auto_(L"test")));
        BOOST_TEST(test("a", auto_('a')));
        BOOST_TEST(test(L"a", auto_(L'a')));

        BOOST_TEST(test("TRUE", upper[auto_(true)]));
        BOOST_TEST(test("TEST", upper[auto_("test")]));

        // test containers
        std::vector<int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(2);
        BOOST_TEST(test("012", auto_(v)));
        BOOST_TEST(test_delimited("0,1,2,", auto_(v), ','));

        std::list<int> l;
        l.push_back(0);
        l.push_back(1);
        l.push_back(2);
        BOOST_TEST(test("012", auto_(l)));
        BOOST_TEST(test_delimited("0,1,2,", auto_(l), ','));

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test("", auto_(o)));
        o = 1;
        BOOST_TEST(test("1", auto_(o)));

        // test alternative
        boost::variant<int, double, float, std::string> vv;
        vv = 1;
        BOOST_TEST(test("1", auto_(vv)));
        vv = 1.0;
        BOOST_TEST(test("1.0", auto_(vv)));
        vv = 1.0f;
        BOOST_TEST(test("1.0", auto_(vv)));
        vv = "some string";
        BOOST_TEST(test("some string", auto_(vv)));

        // test fusion sequence
        std::pair<int, double> p (1, 2.0);
        BOOST_TEST(test("12.0", auto_, p));
        BOOST_TEST(test_delimited("1,2.0,", auto_(p), ','));
    }

    return boost::report_errors();
}
