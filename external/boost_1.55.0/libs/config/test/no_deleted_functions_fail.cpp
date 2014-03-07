//  This file was automatically generated on Thu Jun 19 16:26:16 2008
//  by libs/config/tools/generate.cpp
//  Copyright John Maddock 2002-4.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.//
//  Revision $Id: no_deleted_functions_fail.cpp 85088 2013-07-20 17:17:10Z andysem $
//


// Test file for macro BOOST_NO_CXX11_DELETED_FUNCTIONS
// This file should not compile, if it does then
// BOOST_NO_CXX11_DELETED_FUNCTIONS should not be defined.
// See file boost_no_deleted_functions.ipp for details

// Must not have BOOST_ASSERT_CONFIG set; it defeats
// the objective of this file:
#ifdef BOOST_ASSERT_CONFIG
#  undef BOOST_ASSERT_CONFIG
#endif

#include <boost/config.hpp>
#include "test.hpp"

#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
#include "boost_no_deleted_functions.ipp"
#else
#error "this file should not compile"
#endif

int main( int, char *[] )
{
   return boost_no_cxx11_deleted_functions::test();
}

