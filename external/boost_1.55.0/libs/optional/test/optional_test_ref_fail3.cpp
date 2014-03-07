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
void optional_reference__test_no_converting_ctor()
{
  boost::optional<short&> opt1 ;
  boost::optional<int&> opt2 = opt1 ;
}


