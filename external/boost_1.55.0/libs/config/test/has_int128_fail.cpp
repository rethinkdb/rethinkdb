//  This file was automatically generated on Thu Oct 25 10:14:36 2012
//  by libs/config/tools/generate.cpp
//  Copyright John Maddock 2002-4.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.//
//  Revision $Id: has_int128_fail.cpp 81827 2012-12-10 17:26:30Z johnmaddock $
//


// Test file for macro BOOST_HAS_INT128
// This file should not compile, if it does then
// BOOST_HAS_INT128 should be defined.
// See file boost_has_int128.ipp for details

// Must not have BOOST_ASSERT_CONFIG set; it defeats
// the objective of this file:
#ifdef BOOST_ASSERT_CONFIG
#  undef BOOST_ASSERT_CONFIG
#endif

#include <boost/config.hpp>
#include "test.hpp"

#ifndef BOOST_HAS_INT128
#include "boost_has_int128.ipp"
#else
#error "this file should not compile"
#endif

int main( int, char *[] )
{
   return boost_has_int128::test();
}

