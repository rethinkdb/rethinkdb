/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "uint.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;

    ///////////////////////////////////////////////////////////////////////////
    //  unsigned integer literal tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::lit;
        unsigned i = 123456;

        BOOST_TEST( test("123456", lit(123456U)));
        BOOST_TEST(!test("123456", lit(0U)));
        BOOST_TEST( test("123456", lit(i)));
        BOOST_TEST(!test("123456", lit(unsigned(i - 1))));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  unsigned long long literal tests
    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    {
        using boost::spirit::lit;
        using boost::ulong_long_type;
        ulong_long_type ll = 1234567890123456789ULL;

        BOOST_TEST( test("1234567890123456789", lit(1234567890123456789ULL)));
        BOOST_TEST(!test("1234567890123456789", lit(0ULL)));
        BOOST_TEST( test("1234567890123456789", lit(ll)));
        BOOST_TEST(!test("1234567890123456789", lit(ulong_long_type(ll - 1))));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  ushort_ and ulong_ literal tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::lit;
        unsigned short s = 12345;
        unsigned long l = 1234567890L;

        BOOST_TEST( test("12345",  lit(s)));
        BOOST_TEST(!test("12345",  lit(s - 1)));
        BOOST_TEST( test("1234567890",  lit(1234567890UL)));
        BOOST_TEST(!test("1234567890",  lit(98765321UL)));
        BOOST_TEST( test("1234567890",  lit(l)));
        BOOST_TEST(!test("1234567890",  lit(l - 1)));
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  literal lazy tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::qi::lit;
        unsigned n = 123, m = 321;

        BOOST_TEST(test("123", lit(ref(n))));
        BOOST_TEST(!test("123", lit(ref(m))));
    }

    return boost::report_errors();
}
