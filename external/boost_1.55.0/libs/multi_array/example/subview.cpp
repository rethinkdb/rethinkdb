// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.


#include "boost/multi_array.hpp"
#include "boost/cstdlib.hpp"

int
main()
{
  const int ndims=3;
  typedef boost::multi_array<int,ndims> array;

  int data[] = {
    0,1,2,3,
    4,5,6,7,
    8,9,10,11,

    12,13,14,15,
    16,17,18,19,
    20,21,22,23
  };
  const int data_size=24;

  array myarray(boost::extents[2][3][4]);
  myarray.assign(data,data+data_size);

  //
  // array_view dims:
  // [base,stride,bound)
  // [0,1,2), [1,1,3), [0,2,4) 
  // 

  typedef array::index_range range;
  array::array_view<ndims>::type myview =
    myarray[boost::indices[range(0,2)][range(1,3)][range(0,4,2)]];

  for (array::index i = 0; i != 2; ++i)
    for (array::index j = 0; j != 2; ++j)
      for (array::index k = 0; k != 2; ++k) 
        assert(myview[i][j][k] == myarray[i][j+1][k*2]);

  return boost::exit_success;
}
