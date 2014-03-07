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
#include "boost/cstdlib.hpp"

int main () {
  // Create a 3D array that is 3 x 4 x 2
  typedef boost::multi_array<double, 3> array;
  array A(boost::extents[3][4][2]);
  // Assign a value to an element in the array
  A[0][0][0] = 3.14;
  assert(A[0][0][0] == 3.14);
  return boost::exit_success;
}
