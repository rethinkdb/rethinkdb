// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//
#include "boost/optional.hpp"

//
// THIS TEST SHOULD FAIL TO COMPILE
//
#if BOOST_WORKAROUND( BOOST_INTEL_CXX_VERSION, <= 700) // Intel C++ 7.0
// Interl C++ 7.0 incorrectly accepts the initialization "boost::optional<int> opt = 3"
// even though the ctor is explicit (c.f. 12.3.1.2), so the test uses another form of
// copy-initialization: argument-passing (8.5.12)
void helper ( boost::optional<int> ) ;
void test_explicit_constructor()
{
  helper(3) ; // ERROR: Ctor is explicit.
}
#else
void test_explicit_constructor()
{
  boost::optional<int> opt = 3 ; // ERROR: Ctor is explicit.
}
#endif


