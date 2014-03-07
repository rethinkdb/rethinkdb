//  Copyright Beman Dawes 2012

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_RANGE_BASED_FOR
//  TITLE:         C++11 ranged-based for statement unavailable
//  DESCRIPTION:   The compiler does not support the C++11 range-based for statement

namespace boost_no_cxx11_range_based_for {

int test()
{
  // example from 6.5.4 The range-based for statement [stmt.ranged]
  int array[5] = { 1, 2, 3, 4, 5 };
  for (int& x : array)
    x *= 2;
  return 0;
}

}
