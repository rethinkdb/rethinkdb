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
// stl_interaction.cpp - Make sure multi_arrays work with STL containers.
//

#include "boost/test/minimal.hpp"

#include "boost/multi_array.hpp"
#include <algorithm>
#include <vector>

int
test_main(int, char*[])
{
  using boost::extents;
  using boost::indices;
  typedef boost::multi_array_types::index_range range;
  typedef boost::multi_array<int,3> array3;
  typedef boost::multi_array<int,2> array2;
  
  typedef std::vector<array3> array3vec;

  int data[] = {
    0,1,2,3,
    4,5,6,7,
    8,9,10,11,

    12,13,14,15,
    16,17,18,19,
    20,21,22,23
  };
  const int data_size = 24;

  int insert[] = {
    99,98,
    97,96,
  };
  const int insert_size = 4;
  array3 myarray(extents[2][3][4]);
  myarray.assign(data,data+data_size);

  array3vec myvec(5,myarray);
  BOOST_CHECK(myarray == myvec[1]);

  array3::array_view<2>::type myview =
    myarray[indices[1][range(0,2)][range(1,3)]];

  array2 filler(extents[2][2]);
  filler.assign(insert,insert+insert_size);

  // Modify a portion of myarray through a view (myview)
  myview = filler;


  myvec.push_back(myarray);

  BOOST_CHECK(myarray != myvec[1]);
  BOOST_CHECK(myarray == myvec[5]);

  return boost::exit_success;
}
