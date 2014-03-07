//  Copyright (c) 2011 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/* force fallback implementation using locks */
#define BOOST_ATOMIC_FORCE_FALLBACK 1

#include <boost/atomic.hpp>
#include <boost/cstdint.hpp>
#include <boost/test/minimal.hpp>

#include "api_test_helpers.hpp"

int test_main(int, char *[])
{
    test_flag_api();

    test_integral_api<char>();
    test_integral_api<signed char>();
    test_integral_api<unsigned char>();
    test_integral_api<boost::uint8_t>();
    test_integral_api<boost::int8_t>();
    test_integral_api<short>();
    test_integral_api<unsigned short>();
    test_integral_api<boost::uint16_t>();
    test_integral_api<boost::int16_t>();
    test_integral_api<int>();
    test_integral_api<unsigned int>();
    test_integral_api<boost::uint32_t>();
    test_integral_api<boost::int32_t>();
    test_integral_api<long>();
    test_integral_api<unsigned long>();
    test_integral_api<boost::uint64_t>();
    test_integral_api<boost::int64_t>();
    test_integral_api<long long>();
    test_integral_api<unsigned long long>();

    test_pointer_api<int>();

    test_enum_api();

    test_struct_api<test_struct<boost::uint8_t> >();
    test_struct_api<test_struct<boost::uint16_t> >();
    test_struct_api<test_struct<boost::uint32_t> >();
    test_struct_api<test_struct<boost::uint64_t> >();

    test_large_struct_api();

    return 0;
}
