// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.


#include <cassert>
#include "boost/multi_array.hpp"
#include "boost/array.hpp"
#include "boost/cstdlib.hpp"

int main () {
  // Create a 3D array that is 3 x 4 x 2
  boost::array<int, 3> shape = {{ 3, 4, 2 }};
  boost::multi_array<double, 3> A(shape);
  typedef boost::multi_array<double, 3>::index index;
  // Assign a value to an element in the array
  boost::array<index, 3> idx = {{ 0, 0, 0 }};
  A(idx) = 3.14;
  assert(A(idx) == 3.14);
  return boost::exit_success;
}
