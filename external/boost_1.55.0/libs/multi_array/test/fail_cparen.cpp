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
// fail_cparen.cpp -
//   Testing const operator() constness.
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"

#include "boost/array.hpp"

int
test_main(int,char*[])
{
  const int ndims=3;
  typedef boost::multi_array<int,ndims> array;

  boost::array<array::size_type,ndims> sma_dims = {{2,3,4}};
  int data[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,
                 14,15,16,17,18,19,20,21,22,23};
  const int data_size = 24;
  array sma(sma_dims);
  sma.assign(data,data+data_size);

  const array& csma = sma;
  boost::array<array::index,ndims> indices = {{0,0,0}};

  // FAIL! Cannot assign to csma
  csma(indices) = 5;

  return boost::exit_success;
}
