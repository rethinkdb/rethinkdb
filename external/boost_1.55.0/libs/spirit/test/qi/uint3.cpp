/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
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
    //  parameterized unsigned tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::uint_;
        unsigned u;

        BOOST_TEST(test("123456", uint_(123456)));
        BOOST_TEST(!test("123456", uint_(654321)));
        BOOST_TEST(test_attr("123456", uint_(123456), u));
        BOOST_TEST(u == 123456);
        BOOST_TEST(!test_attr("123456", uint_(654321), u));

        BOOST_TEST(test(max_unsigned, uint_(UINT_MAX)));
        BOOST_TEST(test_attr(max_unsigned, uint_(UINT_MAX), u));
        BOOST_TEST(u == UINT_MAX);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized binary tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::bin;
        unsigned u;

        BOOST_TEST(test("11111110", bin(0xFE)));
        BOOST_TEST(!test("11111110", bin(0xEF)));
        BOOST_TEST(test_attr("11111110", bin(0xFE), u));
        BOOST_TEST(u == 0xFE);
        BOOST_TEST(!test_attr("11111110", bin(0xEF), u));

        BOOST_TEST(test(max_binary, bin(UINT_MAX)));
        BOOST_TEST(test_attr(max_binary, bin(UINT_MAX), u));
        BOOST_TEST(u == UINT_MAX);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized octal tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::oct;
        unsigned u;

        BOOST_TEST(test("12545674515", oct(012545674515)));
#if UINT_MAX > 4294967296 // > 32 bits only
        BOOST_TEST(!test("12545674515", oct(051547654521)));
#endif
        BOOST_TEST(test_attr("12545674515", oct(012545674515), u));
        BOOST_TEST(u == 012545674515);
#if UINT_MAX > 4294967296 // > 32 bits only
        BOOST_TEST(!test_attr("12545674515", oct(051547654521), u));
#endif

        BOOST_TEST(test(max_octal, oct(UINT_MAX)));
        BOOST_TEST(test_attr(max_octal, oct(UINT_MAX), u));
        BOOST_TEST(u == UINT_MAX);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized hex tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::hex;
        unsigned u;

        BOOST_TEST(test("95BC8DF", hex(0x95BC8DF)));
        BOOST_TEST(!test("95BC8DF", hex(0xFD8CB59)));
        BOOST_TEST(test_attr("95BC8DF", hex(0x95BC8DF), u));
        BOOST_TEST(u == 0x95BC8DF);
        BOOST_TEST(!test_attr("95BC8DF", hex(0xFD8CB59), u));

        BOOST_TEST(test("abcdef12", hex(0xabcdef12)));
        BOOST_TEST(!test("abcdef12", hex(0x12fedcba)));
        BOOST_TEST(test_attr("abcdef12", hex(0xabcdef12), u));
        BOOST_TEST(u == 0xabcdef12);
        BOOST_TEST(!test_attr("abcdef12", hex(0x12fedcba), u));

        BOOST_TEST(test(max_hex, hex(UINT_MAX)));
        BOOST_TEST(test_attr(max_hex, hex(UINT_MAX), u));
        BOOST_TEST(u == UINT_MAX);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized action tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::ascii::space;
        using boost::spirit::qi::uint_;
        using boost::spirit::qi::_1;
        unsigned n, m;

        BOOST_TEST(test("123", uint_(123)[ref(n) = _1]));
        BOOST_TEST(n == 123);
        BOOST_TEST(!test("123", uint_(321)[ref(n) = _1]));

        BOOST_TEST(test_attr("789", uint_(789)[ref(n) = _1], m));
        BOOST_TEST(n == 789 && m == 789);
        BOOST_TEST(!test_attr("789", uint_(987)[ref(n) = _1], m));

        BOOST_TEST(test("   456", uint_(456)[ref(n) = _1], space));
        BOOST_TEST(n == 456);
        BOOST_TEST(!test("   456", uint_(654)[ref(n) = _1], space));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized lazy tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::qi::uint_;
        unsigned n = 123, m = 321;

        BOOST_TEST(test("123", uint_(ref(n))));
        BOOST_TEST(!test("123", uint_(ref(m))));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized custom uint tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::uint_;
        using boost::spirit::qi::uint_parser;
        custom_uint u;

        BOOST_TEST(test_attr("123456", uint_(123456), u));
        uint_parser<custom_uint, 10, 1, 2> uint2;
        BOOST_TEST(test_attr("12", uint2(12), u));
    }

    return boost::report_errors();
}
