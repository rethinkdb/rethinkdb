// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

// 
// fail_ref_csubarray.cpp -
//   Testing subarray and const_subarray assignment 
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"

#include "boost/array.hpp"

int
test_main(int,char*[])
{
  const int ndims=3;
  typedef boost::multi_array_ref<int,ndims> array_ref;

  boost::array<array_ref::size_type,ndims> sma_dims = {{2,3,4}};

  int data[] = {77,77,77,77,77,77,77,77,77,77,77,77, 
                 77,77,77,77,77,77,77,77,77,77,77,77}; 

  array_ref sma(data,sma_dims);

  int num = 0;
  for (array_ref::index i = 0; i != 2; ++i)
    for (array_ref::index j = 0; j != 3; ++j)
      for (array_ref::index k = 0; k != 4; ++k)
        sma[i][j][k] = num++;

  array_ref::const_subarray<ndims-1>::type csba = sma[0];

  array_ref::subarray<ndims-1>::type sba = csba; // FAIL! preserve constness.

  return boost::exit_success;
}
