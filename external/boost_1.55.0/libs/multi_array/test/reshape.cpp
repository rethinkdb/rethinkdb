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
// reshape.cpp - testing reshaping functionality
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"

#include "boost/array.hpp"
#include "boost/type.hpp"

int
test_main(int,char*[])
{
  const int ndims=3;
  typedef boost::multi_array<int,ndims> array;
  typedef boost::multi_array_ref<int,ndims> array_ref;
  typedef boost::const_multi_array_ref<int,ndims> const_array_ref;

  boost::array<array::size_type,ndims> dims = {{2,3,4}};
  boost::array<array::size_type,ndims> new_dims = {{4,3,2}};

  int data[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,
                 14,15,16,17,18,19,20,21,22,23};
  const int data_size=24;

  // Basic reshape test
  {
    array A(dims);
    A.assign(data,data+data_size);

    array_ref B(data,dims);
    const_array_ref C(data,dims);

    A.reshape(new_dims);
    B.reshape(new_dims);
    C.reshape(new_dims);

    int* ptr = data;
    for (array::index i = 0; i != 4; ++i)
      for (array::index j = 0; j != 3; ++j)
        for (array::index k = 0; k != 2; ++k) {
          BOOST_CHECK(A[i][j][k] == *ptr);
          BOOST_CHECK(B[i][j][k] == *ptr);
          BOOST_CHECK(C[i][j][k] == *ptr++);
        }
  }

  // Ensure that index bases are preserved over reshape
  {
    boost::array<array::index,ndims> bases = {{0, 1, -1}};

    array A(dims);
    A.assign(data,data+data_size);

    array_ref B(data,dims);
    const_array_ref C(data,dims);

    A.reindex(bases);
    B.reindex(bases);
    C.reindex(bases);

    A.reshape(new_dims);
    B.reshape(new_dims);
    C.reshape(new_dims);

    int* ptr = data;
    for (array::index i = 0; i != 4; ++i)
      for (array::index j = 1; j != 4; ++j)
        for (array::index k = -1; k != 1; ++k) {
          BOOST_CHECK(A[i][j][k] == *ptr);
          BOOST_CHECK(B[i][j][k] == *ptr);
          BOOST_CHECK(C[i][j][k] == *ptr++);
        }
  }
  
  return boost::exit_success;
}







