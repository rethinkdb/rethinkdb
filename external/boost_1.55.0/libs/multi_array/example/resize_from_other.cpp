// Copyright 2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

//
// resize_from_other.cpp - an experiment in writing a resize function for
// multi_arrays that will use the extents from another to build itself.
//

#include <boost/multi_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/array.hpp>
#include <algorithm>

template <typename T, typename U, std::size_t N>
void
resize_from_MultiArray(boost::multi_array<T,N>& marray, U& other) {

  // U must be a model of MultiArray
  boost::function_requires<
  boost::multi_array_concepts::ConstMultiArrayConcept<U,U::dimensionality> >();
  // U better have U::dimensionality == N
  BOOST_STATIC_ASSERT(U::dimensionality == N);

  boost::array<typename boost::multi_array<T,N>::size_type, N> shape;

  std::copy(other.shape(), other.shape()+N, shape.begin());

  marray.resize(shape);
  
}

#include <iostream>


int main () {

  boost::multi_array<int,2> A(boost::extents[5][4]), B;
  boost::multi_array<int,3> C;

  resize_from_MultiArray(B,A);

#if 0
  resize_from_MultiArray(C,A); // Compile-time error
#endif

  std::cout << B.shape()[0] << ", " << B.shape()[1] << '\n';

}
