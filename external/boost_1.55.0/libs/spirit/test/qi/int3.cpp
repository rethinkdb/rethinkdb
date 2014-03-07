/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "int.hpp"
#include <boost/spirit/include/qi_rule.hpp>

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized signed integer tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::int_;
        int i;

        BOOST_TEST(test("123456", int_(123456)));
        BOOST_TEST(!test("123456", int_(654321)));
        BOOST_TEST(test_attr("123456", int_(123456), i));
        BOOST_TEST(i == 123456);
        BOOST_TEST(!test_attr("123456", int_(654321), i));

        BOOST_TEST(test("+123456", int_(123456)));
        BOOST_TEST(!test("+123456", int_(654321)));
        BOOST_TEST(test_attr("+123456", int_(123456), i));
        BOOST_TEST(i == 123456);
        BOOST_TEST(!test_attr("+123456", int_(654321), i));

        BOOST_TEST(test("-123456", int_(-123456)));
        BOOST_TEST(!test("-123456", int_(123456)));
        BOOST_TEST(test_attr("-123456", int_(-123456), i));
        BOOST_TEST(i == -123456);
        BOOST_TEST(!test_attr("-123456", int_(123456), i));

        BOOST_TEST(test(max_int, int_(INT_MAX)));
        BOOST_TEST(test_attr(max_int, int_(INT_MAX), i));
        BOOST_TEST(i == INT_MAX);

        BOOST_TEST(test(min_int, int_(INT_MIN)));
        BOOST_TEST(test_attr(min_int, int_(INT_MIN), i));
        BOOST_TEST(i == INT_MIN);

        // with leading zeros
        BOOST_TEST(test("0000000000123456", int_(123456)));
        BOOST_TEST(test_attr("0000000000123456", int_(123456), i));
        BOOST_TEST(i == 123456);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized long long tests
    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    {
        using boost::spirit::long_long;
        boost::long_long_type ll;

        BOOST_TEST(test("1234567890123456789"
          , long_long(1234567890123456789LL)));
        BOOST_TEST(!test("1234567890123456789"
          , long_long(0)));
        BOOST_TEST(test_attr("1234567890123456789"
          , long_long(1234567890123456789LL), ll));
        BOOST_TEST(ll == 1234567890123456789LL);
        BOOST_TEST(!test_attr("1234567890123456789"
          , long_long(0), ll));

        BOOST_TEST(test("-1234567890123456789"
          , long_long(-1234567890123456789LL)));
        BOOST_TEST(!test("-1234567890123456789"
          , long_long(1234567890123456789LL)));
        BOOST_TEST(test_attr("-1234567890123456789"
          , long_long(-1234567890123456789LL), ll));
        BOOST_TEST(ll == -1234567890123456789LL);
        BOOST_TEST(!test_attr("-1234567890123456789"
          , long_long(1234567890123456789LL), ll));

        BOOST_TEST(test(max_long_long, long_long(LONG_LONG_MAX)));
        BOOST_TEST(test_attr(max_long_long, long_long(LONG_LONG_MAX), ll));
        BOOST_TEST(ll == LONG_LONG_MAX);

        BOOST_TEST(test(min_long_long, long_long(LONG_LONG_MIN)));
        BOOST_TEST(test_attr(min_long_long, long_long(LONG_LONG_MIN), ll));
        BOOST_TEST(ll == LONG_LONG_MIN);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized short_ and long_ tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::short_;
        using boost::spirit::long_;
        int i;

        BOOST_TEST(test("12345", short_(12345)));
        BOOST_TEST(!test("12345", short_(54321)));
        BOOST_TEST(test_attr("12345", short_(12345), i));
        BOOST_TEST(i == 12345);
        BOOST_TEST(!test_attr("12345", short_(54321), i));

        BOOST_TEST(test("1234567890", long_(1234567890L)));
        BOOST_TEST(!test("1234567890", long_(987654321L)));
        BOOST_TEST(test_attr("1234567890", long_(1234567890L), i));
        BOOST_TEST(i == 1234567890);
        BOOST_TEST(!test_attr("1234567890", long_(987654321L), i));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized action tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::ascii::space;
        using boost::spirit::qi::int_;
        using boost::spirit::qi::_1;
        int n, m;

        BOOST_TEST(test("123", int_(123)[ref(n) = _1]));
        BOOST_TEST(n == 123);
        BOOST_TEST(!test("123", int_(321)[ref(n) = _1]));

        BOOST_TEST(test_attr("789", int_(789)[ref(n) = _1], m));
        BOOST_TEST(n == 789 && m == 789);
        BOOST_TEST(!test_attr("789", int_(987)[ref(n) = _1], m));

        BOOST_TEST(test("   456", int_(456)[ref(n) = _1], space));
        BOOST_TEST(n == 456);
        BOOST_TEST(!test("   456", int_(654)[ref(n) = _1], space));
    }

    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::int_;
        using boost::spirit::qi::_1;
        using boost::spirit::qi::_val;
        using boost::spirit::qi::space;

        int i = 0;
        int j = 0;
        BOOST_TEST(test_attr("456", int_[_val = _1], i) && i == 456);
        BOOST_TEST(test_attr("   456", int_[_val = _1], j, space) && j == 456);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized lazy tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::qi::int_;
        int n = 123, m = 321;

        BOOST_TEST(test("123", int_(ref(n))));
        BOOST_TEST(!test("123", int_(ref(m))));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized custom int tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::int_;
        using boost::spirit::qi::int_parser;
        custom_int i;

        BOOST_TEST(test_attr("-123456", int_(-123456), i));
        int_parser<custom_int, 10, 1, 2> int2;
        BOOST_TEST(test_attr("-12", int2(-12), i));
    }

    return boost::report_errors();
}
