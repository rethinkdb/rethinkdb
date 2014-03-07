//  This file was automatically generated on Wed Jun  1 21:44:56 2011
//  by libs/config/tools/generate.cpp
//  Copyright John Maddock 2002-4.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.//
//  Revision $Id: no_decltype_n3276_fail.cpp 79537 2012-07-15 15:59:05Z marshall $
//


// Test file for macro BOOST_NO_CXX11_DECLTYPE_N3276
// This file should not compile, if it does then
// BOOST_NO_CXX11_DECLTYPE_N3276 should not be defined.
// See file boost_no_decltype_n3276.ipp for details

// Must not have BOOST_ASSERT_CONFIG set; it defeats
// the objective of this file:
#ifdef BOOST_ASSERT_CONFIG
#  undef BOOST_ASSERT_CONFIG
#endif

#include <boost/config.hpp>
#include "test.hpp"

#ifdef BOOST_NO_CXX11_DECLTYPE_N3276
#include "boost_no_decltype_n3276.ipp"
#else
#error "this file should not compile"
#endif

int main( int, char *[] )
{
   return boost_no_decltype_n3276::test();
}

