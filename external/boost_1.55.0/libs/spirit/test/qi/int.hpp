/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TEST_QI_INT_HPP)
#define BOOST_SPIRIT_TEST_QI_INT_HPP

#include <climits>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  *** BEWARE PLATFORM DEPENDENT!!! ***
//  *** The following assumes 32 bit or 64 bit integers and 64 bit long longs.
//  *** Modify these constant strings when appropriate.
//
///////////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
// Some compilers have long long, but don't define the
// LONG_LONG_MIN and LONG_LONG_MAX macros in limits.h.  This
// assumes that long long is 64 bits.

BOOST_STATIC_ASSERT(sizeof(boost::long_long_type) == 8);

#if !defined(LONG_LONG_MIN) && !defined(LONG_LONG_MAX)
# define LONG_LONG_MAX 0x7fffffffffffffffLL
# define LONG_LONG_MIN (-LONG_LONG_MAX - 1)
#endif

#endif // BOOST_HAS_LONG_LONG

#if INT_MAX != LLONG_MAX
    BOOST_STATIC_ASSERT(sizeof(int) == 4);
    char const* max_int = "2147483647";
    char const* int_overflow = "2147483648";
    char const* min_int = "-2147483648";
    char const* int_underflow = "-2147483649";
#else
    BOOST_STATIC_ASSERT(sizeof(int) == 8);
    char const* max_int = "9223372036854775807";
    char const* int_overflow = "9223372036854775808";
    char const* min_int = "-9223372036854775808";
    char const* int_underflow = "-9223372036854775809";
#endif

#ifdef BOOST_HAS_LONG_LONG
    char const* max_long_long = "9223372036854775807";
    char const* long_long_overflow = "9223372036854775808";
    char const* min_long_long = "-9223372036854775808";
    char const* long_long_underflow = "-9223372036854775809";
#endif

///////////////////////////////////////////////////////////////////////////////
// A custom int type
struct custom_int
{
    int n;
    custom_int() : n(0) {}
    explicit custom_int(int n_) : n(n_) {}
    custom_int& operator=(int n_) { n = n_; return *this; }
    friend bool operator==(custom_int a, custom_int b) { return a.n == b.n; }
    friend bool operator==(custom_int a, int b) { return a.n == b; }
    friend custom_int operator*(custom_int a, custom_int b) { return custom_int(a.n * b.n); }
    friend custom_int operator+(custom_int a, custom_int b) { return custom_int(a.n + b.n); }
    friend custom_int operator-(custom_int a, custom_int b) { return custom_int(a.n - b.n); }
};

#endif 
