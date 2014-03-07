//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_binary.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;
    using namespace boost::phoenix;

    {   // test native endian binaries
#ifdef BOOST_LITTLE_ENDIAN
        BOOST_TEST(binary_test("\x01", 1, byte_, 0x01));
        BOOST_TEST(binary_test("\x80", 1, byte_, 0x80));
        BOOST_TEST(binary_test("\x01\x82", 2, word, 0x8201));
        BOOST_TEST(binary_test("\x81\x02", 2, word, 0x0281));
        BOOST_TEST(binary_test("\x01\x02\x03\x84", 4, dword, 0x84030201));
        BOOST_TEST(binary_test("\x81\x02\x03\x04", 4, dword, 0x04030281));
#ifdef BOOST_HAS_LONG_LONG
        BOOST_TEST(binary_test("\x01\x02\x03\x04\x05\x06\x07\x88", 8, qword,
            0x8807060504030201LL));
        BOOST_TEST(binary_test("\x81\x02\x03\x04\x05\x06\x07\x08", 8, qword,
            0x0807060504030281LL));
#endif
        BOOST_TEST(binary_test("\x00\x00\x80\x3f", 4, bin_float, 1.0f));
        BOOST_TEST(binary_test("\x00\x00\x00\x00\x00\x00\xf0\x3f", 8,
            bin_double, 1.0));

        BOOST_TEST(binary_test_delimited("\x01\x00\x00\x00", 4, byte_, 0x01, pad(4)));
        BOOST_TEST(binary_test_delimited("\x01\x02\x00\x00", 4, word, 0x0201, pad(4)));
        BOOST_TEST(binary_test_delimited("\x01\x02\x03\x04", 4, dword, 0x04030201, pad(4)));
#ifdef BOOST_HAS_LONG_LONG
        BOOST_TEST(binary_test_delimited("\x01\x02\x03\x04\x05\x06\x07\x08\x00\x00", 10, 
            qword, 0x0807060504030201LL, pad(10)));
#endif
        BOOST_TEST(binary_test_delimited("\x00\x00\x80\x3f", 4, bin_float,
            1.0f, pad(4)));
        BOOST_TEST(binary_test_delimited("\x00\x00\x00\x00\x00\x00\xf0\x3f", 8,
            bin_double, 1.0, pad(8)));

#else // BOOST_LITTLE_ENDIAN

        BOOST_TEST(binary_test("\x01", 1, byte_, 0x01));
        BOOST_TEST(binary_test("\x80", 1, byte_, 0x80));
        BOOST_TEST(binary_test("\x01\x82", 2, word, 0x0182));
        BOOST_TEST(binary_test("\x81\x02", 2, word, 0x8102));
        BOOST_TEST(binary_test("\x01\x02\x03\x84", 4, dword, 0x01020384));
        BOOST_TEST(binary_test("\x81\x02\x03\x04", 4, dword, 0x81020304));
#ifdef BOOST_HAS_LONG_LONG
        BOOST_TEST(binary_test("\x01\x02\x03\x04\x05\x06\x07\x88", 8, qword,
            0x0102030405060788LL));
        BOOST_TEST(binary_test("\x81\x02\x03\x04\x05\x06\x07\x08", 8, qword,
            0x8102030405060708LL));
#endif
        BOOST_TEST(binary_test("\x3f\x80\x00\x00", 4, bin_float, 1.0f));
        BOOST_TEST(binary_test("\x3f\xf0\x00\x00\x00\x00\x00\x00", 8,
            bin_double, 1.0));

        BOOST_TEST(binary_test_delimited("\x01\x00\x00\x00", 4, byte_, 0x01, pad(4)));
        BOOST_TEST(binary_test_delimited("\x01\x02\x00\x00", 4, word, 0x0102, pad(4)));
        BOOST_TEST(binary_test_delimited("\x01\x02\x03\x04", 4, dword, 0x01020304, pad(4)));
#ifdef BOOST_HAS_LONG_LONG
        BOOST_TEST(binary_test_delimited("\x01\x02\x03\x04\x05\x06\x07\x08\x00\x00", 10, 
            qword, 0x0102030405060708LL, pad(10)));
#endif
        BOOST_TEST(binary_test_delimited("\x3f\x80\x00\x00", 4, bin_float,
            1.0f, pad(4)));
        BOOST_TEST(binary_test_delimited("\x3f\xf0\x00\x00\x00\x00\x00\x00", 8,
            bin_double, 1.0, pad(8)));
#endif
    }

    {   // test native endian binaries
#ifdef BOOST_LITTLE_ENDIAN
        BOOST_TEST(binary_test("\x01", 1, byte_(0x01)));
        BOOST_TEST(binary_test("\x01\x02", 2, word(0x0201)));
        BOOST_TEST(binary_test("\x01\x02\x03\x04", 4, dword(0x04030201)));
#ifdef BOOST_HAS_LONG_LONG
        BOOST_TEST(binary_test("\x01\x02\x03\x04\x05\x06\x07\x08", 8,
            qword(0x0807060504030201LL)));
#endif
        BOOST_TEST(binary_test("\x00\x00\x80\x3f", 4, bin_float(1.0f)));
        BOOST_TEST(binary_test("\x00\x00\x00\x00\x00\x00\xf0\x3f", 8,
            bin_double(1.0)));
#else
        BOOST_TEST(binary_test("\x01", 1, byte_(0x01)));
        BOOST_TEST(binary_test("\x01\x02", 2, word(0x0102)));
        BOOST_TEST(binary_test("\x01\x02\x03\x04", 4, dword(0x01020304)));
#ifdef BOOST_HAS_LONG_LONG
        BOOST_TEST(binary_test("\x01\x02\x03\x04\x05\x06\x07\x08", 8,
            qword(0x0102030405060708LL)));
#endif
        BOOST_TEST(binary_test("\x3f\x80\x00\x00", 4, bin_float(1.0f)));
        BOOST_TEST(binary_test("\x3f\xf0\x00\x00\x00\x00\x00\x00", 8,
            bin_double(1.0)));
#endif
    }

    return boost::report_errors();
}
