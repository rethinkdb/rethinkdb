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
    //  signed integer literal tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::lit;
        int i = 123456;

        BOOST_TEST( test("123456", lit(123456)));
        BOOST_TEST(!test("123456", lit(0)));
        BOOST_TEST( test("123456", lit(i)));
        BOOST_TEST(!test("123456", lit(-i)));
        BOOST_TEST( test("+425", lit(425)));
        BOOST_TEST(!test("+425", lit(17)));
        BOOST_TEST( test("-2000", lit(-2000)));
        BOOST_TEST(!test("-2000", lit(2000)));
        BOOST_TEST( test(max_int, lit(INT_MAX)));
        BOOST_TEST(!test(max_int, lit(INT_MIN)));

        BOOST_TEST( test(min_int, lit(INT_MIN)));
        BOOST_TEST(!test(min_int, lit(INT_MAX)));

        BOOST_TEST(!test("-", lit(8451)));
        BOOST_TEST(!test("+", lit(8451)));

        // with leading zeros
        BOOST_TEST(test("000000000098765", lit(98765)));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  long long literal tests
    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    {
        using boost::spirit::lit;
        boost::long_long_type ll = 1234567890123456789LL;

        BOOST_TEST( test("1234567890123456789", lit(1234567890123456789LL)));
        BOOST_TEST(!test("1234567890123456789", lit(-19LL)));
        BOOST_TEST( test("1234567890123456789", lit(ll)));
        BOOST_TEST(!test("1234567890123456789", lit(-ll)));
        BOOST_TEST( test("-100000000000000", lit(-100000000000000LL)));
        BOOST_TEST(!test("-100000000000000", lit(3243515525263LL)));
        BOOST_TEST( test(max_long_long, lit(LONG_LONG_MAX)));
        BOOST_TEST(!test(max_long_long, lit(LONG_LONG_MIN)));

        BOOST_TEST( test(min_long_long, lit(LONG_LONG_MIN)));
        BOOST_TEST(!test(min_long_long, lit(LONG_LONG_MAX)));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  short_ and long_ literal tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::lit;
        short s = 12345;
        long l = 1234567890L;

        BOOST_TEST( test("12345",  lit(12345)));
        BOOST_TEST(!test("12345",  lit(-12345)));
        BOOST_TEST( test("12345",  lit(s)));
        BOOST_TEST(!test("12345",  lit(-s)));
        BOOST_TEST( test("-12345", lit(-12345)));
        BOOST_TEST(!test("-12345", lit(12345)));
        BOOST_TEST( test("-12345", lit(-s)));
        BOOST_TEST(!test("-12345", lit(s)));

        BOOST_TEST( test("1234567890",  lit(1234567890)));
        BOOST_TEST(!test("1234567890",  lit(-1234567890)));
        BOOST_TEST( test("1234567890",  lit(l)));
        BOOST_TEST(!test("1234567890",  lit(-l)));
        BOOST_TEST( test("-1234567890", lit(-1234567890)));
        BOOST_TEST(!test("-1234567890", lit(1234567890)));
        BOOST_TEST( test("-1234567890", lit(-l)));
        BOOST_TEST(!test("-1234567890", lit(l)));
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  literal lazy tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::qi::lit;
        int n = 123, m = 321;

        BOOST_TEST(test("123", lit(ref(n))));
        BOOST_TEST(!test("123", lit(ref(m))));
    }

    return boost::report_errors();
}
