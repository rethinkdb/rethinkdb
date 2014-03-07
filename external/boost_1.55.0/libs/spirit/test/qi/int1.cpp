/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "int.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;

    ///////////////////////////////////////////////////////////////////////////
    //  signed integer tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::int_;
        int i;

        BOOST_TEST(test("123456", int_));
        BOOST_TEST(test_attr("123456", int_, i));
        BOOST_TEST(i == 123456);

        BOOST_TEST(test("+123456", int_));
        BOOST_TEST(test_attr("+123456", int_, i));
        BOOST_TEST(i == 123456);

        BOOST_TEST(test("-123456", int_));
        BOOST_TEST(test_attr("-123456", int_, i));
        BOOST_TEST(i == -123456);

        BOOST_TEST(test(max_int, int_));
        BOOST_TEST(test_attr(max_int, int_, i));
        BOOST_TEST(i == INT_MAX);

        BOOST_TEST(test(min_int, int_));
        BOOST_TEST(test_attr(min_int, int_, i));
        BOOST_TEST(i == INT_MIN);

        BOOST_TEST(!test(int_overflow, int_));
        BOOST_TEST(!test_attr(int_overflow, int_, i));
        BOOST_TEST(!test(int_underflow, int_));
        BOOST_TEST(!test_attr(int_underflow, int_, i));

        BOOST_TEST(!test("-", int_));
        BOOST_TEST(!test_attr("-", int_, i));

        BOOST_TEST(!test("+", int_));
        BOOST_TEST(!test_attr("+", int_, i));

        // Bug report from Steve Nutt
        BOOST_TEST(!test_attr("5368709120", int_, i));

        // with leading zeros
        BOOST_TEST(test("0000000000123456", int_));
        BOOST_TEST(test_attr("0000000000123456", int_, i));
        BOOST_TEST(i == 123456);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  long long tests
    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    {
        using boost::spirit::long_long;
        boost::long_long_type ll;

        BOOST_TEST(test("1234567890123456789", long_long));
        BOOST_TEST(test_attr("1234567890123456789", long_long, ll));
        BOOST_TEST(ll == 1234567890123456789LL);

        BOOST_TEST(test("-1234567890123456789", long_long));
        BOOST_TEST(test_attr("-1234567890123456789", long_long, ll));
        BOOST_TEST(ll == -1234567890123456789LL);

        BOOST_TEST(test(max_long_long, long_long));
        BOOST_TEST(test_attr(max_long_long, long_long, ll));
        BOOST_TEST(ll == LONG_LONG_MAX);

        BOOST_TEST(test(min_long_long, long_long));
        BOOST_TEST(test_attr(min_long_long, long_long, ll));
        BOOST_TEST(ll == LONG_LONG_MIN);

        BOOST_TEST(!test(long_long_overflow, long_long));
        BOOST_TEST(!test_attr(long_long_overflow, long_long, ll));
        BOOST_TEST(!test(long_long_underflow, long_long));
        BOOST_TEST(!test_attr(long_long_underflow, long_long, ll));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  short_ and long_ tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::short_;
        using boost::spirit::long_;
        int i;

        BOOST_TEST(test("12345", short_));
        BOOST_TEST(test_attr("12345", short_, i));
        BOOST_TEST(i == 12345);

        BOOST_TEST(test("1234567890", long_));
        BOOST_TEST(test_attr("1234567890", long_, i));
        BOOST_TEST(i == 1234567890);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Check overflow is parse error
    ///////////////////////////////////////////////////////////////////////////
    {
        boost::spirit::qi::int_parser<boost::int8_t> int8_;
        char c;

        BOOST_TEST(!test_attr("999", int8_, c));

        int i;
        using boost::spirit::short_;
        BOOST_TEST(!test_attr("32769", short_, i, false));
        BOOST_TEST(!test_attr("41234", short_, i, false));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  int_parser<unused_type> tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::int_parser;
        using boost::spirit::unused_type;
        int_parser<unused_type> any_int;

        BOOST_TEST(test("123456", any_int));
        BOOST_TEST(test("-123456", any_int));
        BOOST_TEST(test("-1234567890123456789", any_int));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  action tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::_1;
        using boost::spirit::ascii::space;
        using boost::spirit::int_;
        int n, m;

        BOOST_TEST(test("123", int_[ref(n) = _1]));
        BOOST_TEST(n == 123);
        BOOST_TEST(test_attr("789", int_[ref(n) = _1], m));
        BOOST_TEST(n == 789 && m == 789);
        BOOST_TEST(test("   456", int_[ref(n) = _1], space));
        BOOST_TEST(n == 456);
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  custom int tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::int_;
        using boost::spirit::qi::int_parser;
        custom_int i;

        BOOST_TEST(test_attr("-123456", int_, i));
        int_parser<custom_int, 10, 1, 2> int2;
        BOOST_TEST(test_attr("-12", int2, i));
    }

    return boost::report_errors();
}
