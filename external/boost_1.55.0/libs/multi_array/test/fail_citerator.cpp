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
// fail_citerator.cpp -
//   const_iterator/iterator conversion  test
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"


int test_main(int,char*[]) {
  typedef boost::multi_array<int,3> array;

  typedef array::iterator iterator1;
  typedef array::const_iterator citerator1;

  // ILLEGAL conversion from const_iterator to iterator
  iterator1 in = citerator1();

  return boost::exit_success;
}
