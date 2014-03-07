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
void test_no_implicit_conversion()
{
  boost::optional<int> opt(1) ;

  // You can compare against 0 or against another optional<>,
  // but not against another value
  if ( opt == 1 ) ;
}


