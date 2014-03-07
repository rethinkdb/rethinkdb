//  This file was automatically generated on Fri Dec 03 18:04:01 2004
//  by libs/config/tools/generate.cpp
//  Copyright John Maddock 2002-4.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

// Test file for macro BOOST_NO_MS_INT64_NUMERIC_LIMITS
// This file should not compile, if it does then
// BOOST_NO_MS_INT64_NUMERIC_LIMITS should not be defined.
// See file boost_no_i64_limits.ipp for details

// Must not have BOOST_ASSERT_CONFIG set; it defeats
// the objective of this file:
#ifdef BOOST_ASSERT_CONFIG
#  undef BOOST_ASSERT_CONFIG
#endif

#include <boost/config.hpp>
#include "test.hpp"

#ifdef BOOST_NO_MS_INT64_NUMERIC_LIMITS
#include "boost_no_i64_limits.ipp"
#else
#error "this file should not compile"
#endif

int main( int, char *[] )
{
   return boost_no_ms_int64_numeric_limits::test();
}

