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
// test out my new storage_order stuff
//

#include "boost/test/minimal.hpp"

#include "boost/multi_array/storage_order.hpp"

int 
test_main(int,char*[]) {

  using namespace boost;

  array<std::size_t,5> c_ordering = {{4,3,2,1,0}};;
  array<std::size_t,5> fortran_ordering = {{0,1,2,3,4}};
  array<bool,5> ascending = {{true,true,true,true,true}};
  general_storage_order<5> c_storage(c_ordering.begin(),
                                     ascending.begin());
  general_storage_order<5> fortran_storage(fortran_ordering.begin(),
                                           ascending.begin());
 
  BOOST_CHECK(c_storage == (general_storage_order<5>) c_storage_order());
  BOOST_CHECK(fortran_storage ==
             (general_storage_order<5>) fortran_storage_order());

  return boost::exit_success;
}
