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
#include<string>

#include "boost/optional.hpp"

struct A {} ;
struct B {} ;
//
// THIS TEST SHOULD FAIL TO COMPILE
//
void test_no_unsupported_conversion()
{
  boost::optional<A> opt1;
  boost::optional<B> opt2;
  opt2 = opt1 ; // Cannot convert from "A" to "B"
}


