//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "auto.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        BOOST_TEST((!traits::meta_create_exists<karma::domain, my_type>::value));
    }

    {
        // test primitive types
        BOOST_TEST(test_create_generator("true", true));
        BOOST_TEST(test_create_generator("1", 1));
        BOOST_TEST(test_create_generator("1.1", 1.1));
        BOOST_TEST(test_create_generator("test", std::string("test")));
        BOOST_TEST(test_create_generator("a", 'a'));
        BOOST_TEST(test_create_generator(L"a", L'a'));

        // test containers
        std::vector<int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(2);
        BOOST_TEST(test_create_generator("012", v));

        std::list<int> l;
        l.push_back(0);
        l.push_back(1);
        l.push_back(2);
        BOOST_TEST(test_create_generator("012", l));

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_create_generator("", o));
        o = 1;
        BOOST_TEST(test_create_generator("1", o));

        // test alternative
        boost::variant<int, double, float, std::string> vv;
        vv = 1;
        BOOST_TEST(test_create_generator("1", vv));
        vv = 1.0;
        BOOST_TEST(test_create_generator("1.0", vv));
        vv = 1.0f;
        BOOST_TEST(test_create_generator("1.0", vv));
        vv = "some string";
        BOOST_TEST(test_create_generator("some string", vv));

        // test fusion sequence
        std::pair<int, double> p (1, 2.0);
        BOOST_TEST(test_create_generator("12.0", p));
    }

    {
        // test primitive types
//         BOOST_TEST(test_create_generator_auto("true", true));
//         BOOST_TEST(test_create_generator_auto("1", 1));
//         BOOST_TEST(test_create_generator_auto("1.1", 1.1));
//         BOOST_TEST(test_create_generator_auto("test", std::string("test")));
//         BOOST_TEST(test_create_generator_auto("a", 'a'));
//         BOOST_TEST(test_create_generator_auto(L"a", L'a'));

        // test containers
        std::vector<int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(2);
        BOOST_TEST(test_create_generator_auto("012", v));

        std::list<int> l;
        l.push_back(0);
        l.push_back(1);
        l.push_back(2);
        BOOST_TEST(test_create_generator_auto("012", l));

        // test optional
        boost::optional<int> o;
        BOOST_TEST(test_create_generator_auto("", o));
        o = 1;
        BOOST_TEST(test_create_generator_auto("1", o));

        // test alternative
        boost::variant<int, double, float, std::string> vv;
        vv = 1;
        BOOST_TEST(test_create_generator_auto("1", vv));
        vv = 1.0;
        BOOST_TEST(test_create_generator_auto("1.0", vv));
        vv = 1.0f;
        BOOST_TEST(test_create_generator_auto("1.0", vv));
        vv = "some string";
        BOOST_TEST(test_create_generator_auto("some string", vv));

        // test fusion sequence
        std::pair<int, double> p (1, 2.0);
        BOOST_TEST(test_create_generator_auto("12.0", p));
    }

    return boost::report_errors();
}
