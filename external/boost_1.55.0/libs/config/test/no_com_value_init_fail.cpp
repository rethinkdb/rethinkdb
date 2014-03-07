//  This file was automatically generated on Fri Apr 09 12:24:53 2010
//  by libs/config/tools/generate.cpp
//  Copyright John Maddock 2002-4.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.//
//  Revision $Id: generate.cpp 49281 2008-10-11 15:40:44Z johnmaddock $
//


// Test file for macro BOOST_NO_COMPLETE_VALUE_INITIALIZATION
// This file should not compile, if it does then
// BOOST_NO_COMPLETE_VALUE_INITIALIZATION should not be defined.
// See file boost_no_com_value_init.ipp for details

// Must not have BOOST_ASSERT_CONFIG set; it defeats
// the objective of this file:
#ifdef BOOST_ASSERT_CONFIG
#  undef BOOST_ASSERT_CONFIG
#endif

#include <boost/config.hpp>
#include "test.hpp"

#ifdef BOOST_NO_COMPLETE_VALUE_INITIALIZATION
#include "boost_no_com_value_init.ipp"
#else
#error "this file should not compile"
#endif

int main( int, char *[] )
{
   return boost_no_complete_value_initialization::test();
}

