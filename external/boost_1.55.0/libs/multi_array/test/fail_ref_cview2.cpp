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
// fail_ref_cview2.cpp
//   ensure const_array_view cannot be converted to array_view
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"

#include "boost/array.hpp"
#include "boost/type.hpp"

int
test_main(int,char*[])
{
  const int ndims=3;
  typedef boost::multi_array_ref<int,ndims> array_ref;

  boost::array<array_ref::size_type,ndims> sma_dims = {{2,3,4}};

  int data[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,
                 14,15,16,17,18,19,20,21,22,23};

  array_ref sma(data,sma_dims);

  //
  // subarray dims:
  // [base,stride,bound)
  // [0,1,2), [1,1,3), [0,2,4) 
  // 
  
  const array_ref& csma = sma;
  typedef array_ref::index_range range;
  array_ref::index_gen indices;
  array_ref::array_view<ndims>::type csma2 =
    csma[indices[range(0,2)][range(1,3)][range(0,4,2)]];

  for (array_ref::index i = 0; i != 2; ++i)
    for (array_ref::index j = 0; j != 2; ++j)
      for (array_ref::index k = 0; k != 2; ++k)
        csma2[i][j][k] = 0; // FAIL! csma2 is read only
  
  return boost::exit_success;
}







