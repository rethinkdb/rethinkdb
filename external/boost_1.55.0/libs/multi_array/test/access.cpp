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
// access.cpp - operator[] and operator() tests with various arrays
//    The tests assume that they are working on an Array of shape 2x3x4
//

#include "generative_tests.hpp"
#include "boost/static_assert.hpp"

template <typename Array>
void access(Array& A, const mutable_array_tag&) {
  assign(A);
  access(A,const_array_tag());

  const Array& CA = A;
  access(CA,const_array_tag());
}

template <typename Array>
void access(Array& A, const const_array_tag&) {
  const unsigned int ndims = 3;
  BOOST_STATIC_ASSERT((Array::dimensionality == ndims));
  typedef typename Array::index index;
  const index idx0 = A.index_bases()[0];
  const index idx1 = A.index_bases()[1];
  const index idx2 = A.index_bases()[2];

  // operator[]
  int cnum = 0;
  const Array& CA = A;
  for (index i = idx0; i != idx0+2; ++i)
    for (index j = idx1; j != idx1+3; ++j)
      for (index k = idx2; k != idx2+4; ++k) {
        BOOST_CHECK(A[i][j][k] == cnum++);
        BOOST_CHECK(CA[i][j][k] == A[i][j][k]);
      }

  // operator()
  for (index i2 = idx0; i2 != idx0+2; ++i2)
    for (index j2 = idx1; j2 != idx1+3; ++j2)
      for (index k2 = idx2; k2 != idx2+4; ++k2) {
        boost::array<index,ndims> indices;
        indices[0] = i2; indices[1] = j2; indices[2] = k2;
        BOOST_CHECK(A(indices) == A[i2][j2][k2]);
        BOOST_CHECK(CA(indices) == A(indices));
      }
  ++tests_run;
}

int test_main(int,char*[]) {
  return run_generative_tests();
}
