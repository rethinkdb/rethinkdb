/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TEST_QI_UINT_HPP)
#define BOOST_SPIRIT_TEST_QI_UINT_HPP

#include <climits>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include "test.hpp"
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
//
//  *** BEWARE PLATFORM DEPENDENT!!! ***
//  *** The following assumes 32 bit integers and 64 bit long longs.
//  *** Modify these constant strings when appropriate.
//
///////////////////////////////////////////////////////////////////////////////

    char const* max_unsigned = "4294967295";
    char const* unsigned_overflow = "4294967296";
    char const* max_int = "2147483647";
    char const* int_overflow = "2147483648";
    char const* min_int = "-2147483648";
    char const* int_underflow = "-2147483649";
    char const* max_binary = "11111111111111111111111111111111";
    char const* binary_overflow = "100000000000000000000000000000000";
    char const* max_octal = "37777777777";
    char const* octal_overflow = "100000000000";
    char const* max_hex = "FFFFFFFF";
    char const* hex_overflow = "100000000";

///////////////////////////////////////////////////////////////////////////////
// A custom int type
struct custom_uint
{
    unsigned n;
    custom_uint() : n(0) {}
    explicit custom_uint(unsigned n_) : n(n_) {}
    custom_uint& operator=(unsigned n_) { n = n_; return *this; }
    friend bool operator==(custom_uint a, custom_uint b)
        { return a.n == b.n; }
    friend bool operator==(custom_uint a, unsigned b)
        { return a.n == b; }
    friend custom_uint operator*(custom_uint a, custom_uint b)
        { return custom_uint(a.n * b.n); }
    friend custom_uint operator+(custom_uint a, custom_uint b)
        { return custom_uint(a.n + b.n); }
};

#endif

