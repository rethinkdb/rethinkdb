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
// storage_order.cpp - testing storage_order-isms.
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"
 
#include "boost/array.hpp"

int
test_main(int,char*[])
{
  const int ndims=3;
  
  int data_row[] = {
    0,1,2,3,
    4,5,6,7,
    8,9,10,11,

    12,13,14,15,
    16,17,18,19,
    20,21,22,23 
  };
                  
  int data_col[] = {
    0,12,
    4,16,
    8,20,

    1,13,
    5,17,
    9,21,

    2,14,
    6,18,
    10,22,

    3,15,
    7,19,
    11,23
  };
  const int num_elements = 24;

  // fortran storage order
  {
    typedef boost::multi_array<int,ndims> array;

    array::extent_gen extents;
    array A(extents[2][3][4],boost::fortran_storage_order());

    A.assign(data_col,data_col+num_elements);
  
    int* num = data_row;
    for (array::index i = 0; i != 2; ++i)
      for (array::index j = 0; j != 3; ++j)
        for (array::index k = 0; k != 4; ++k)
          BOOST_CHECK(A[i][j][k] == *num++);
  }

  // Mimic fortran_storage_order using
  // general_storage_order data placement
  {
    typedef boost::general_storage_order<ndims> storage;
    typedef boost::multi_array<int,ndims> array;

    array::size_type ordering[] = {0,1,2};
    bool ascending[] = {true,true,true};
  
    array::extent_gen extents;
    array A(extents[2][3][4], storage(ordering,ascending));

    A.assign(data_col,data_col+num_elements);
  
    int* num = data_row;
    for (array::index i = 0; i != 2; ++i)
      for (array::index j = 0; j != 3; ++j)
        for (array::index k = 0; k != 4; ++k)
          BOOST_CHECK(A[i][j][k] == *num++);
  }

  // general_storage_order with arbitrary storage order
  {
    typedef boost::general_storage_order<ndims> storage;
    typedef boost::multi_array<int,ndims> array;

    array::size_type ordering[] = {2,0,1};
    bool ascending[] = {true,true,true};
  
    array::extent_gen extents;
    array A(extents[2][3][4], storage(ordering,ascending));

    int data_arb[] = {
      0,1,2,3,
      12,13,14,15,

      4,5,6,7,
      16,17,18,19,

      8,9,10,11,
      20,21,22,23 
    };

    A.assign(data_arb,data_arb+num_elements);
  
    int* num = data_row;
    for (array::index i = 0; i != 2; ++i)
      for (array::index j = 0; j != 3; ++j)
        for (array::index k = 0; k != 4; ++k)
          BOOST_CHECK(A[i][j][k] == *num++);
  }


  // general_storage_order with descending dimensions.
  {
    const int ndims=3;
    typedef boost::general_storage_order<ndims> storage;
    typedef boost::multi_array<int,ndims> array;

    array::size_type ordering[] = {2,0,1};
    bool ascending[] = {false,true,true};
  
    array::extent_gen extents;
    array A(extents[2][3][4], storage(ordering,ascending));

    int data_arb[] = {
      12,13,14,15,
      0,1,2,3,

      16,17,18,19,
      4,5,6,7,

      20,21,22,23, 
      8,9,10,11
    };

    A.assign(data_arb,data_arb+num_elements);
  
    int* num = data_row;
    for (array::index i = 0; i != 2; ++i)
      for (array::index j = 0; j != 3; ++j)
        for (array::index k = 0; k != 4; ++k)
          BOOST_CHECK(A[i][j][k] == *num++);
  }

  return boost::exit_success;
}
