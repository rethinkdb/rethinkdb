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
// idxset1.cpp - testing the code for index_gen
//

#include "boost/multi_array/index_gen.hpp"
#include "boost/multi_array/index_range.hpp"
#include "boost/multi_array/types.hpp"
#include "boost/test/minimal.hpp"

#include "boost/array.hpp"

typedef boost::detail::multi_array::index index_type;
typedef boost::detail::multi_array::size_type size_type;
typedef boost::detail::multi_array::index_range<index_type,size_type> range;

template <int NumRanges, int NumDims>
void check(const boost::detail::multi_array::
           index_gen<NumRanges,NumDims>&) { }

bool operator==(const range& lhs,const range& rhs) {  
  return lhs.start_ == rhs.start_ &&
    lhs.finish_ == rhs.finish_ &&
    lhs.stride_ == rhs.stride_ &&
    lhs.degenerate_ == rhs.degenerate_;
}

int
test_main(int,char*[])
{

  boost::detail::multi_array::index_gen<0,0> indices;


  check<1,1>(indices[range()]);
  check<2,2>(indices[range()][range()]);
  check<3,3>(indices[range()][range()][range()]);

  check<1,0>(indices[0]);
  check<2,0>(indices[0][0]);
  check<2,1>(indices[range()][0]);
  check<2,1>(indices[0][range()]);
  check<3,0>(indices[0][0][0]);
  check<3,1>(indices[range()][0][0]);
  check<3,1>(indices[0][range()][0]);
  check<3,1>(indices[0][0][range()]);
  check<3,2>(indices[range()][range()][0]);
  check<3,2>(indices[range()][0][range()]);
  check<3,2>(indices[0][range()][range()]);

  {
    boost::detail::multi_array::index_gen<3,3> is1 =
      indices[range(0,1,2)][range(1,2,3)][range(2,3,4)];
    BOOST_CHECK(is1.ranges_[0] == range(0,1,2));
    BOOST_CHECK(is1.ranges_[1] == range(1,2,3));
    BOOST_CHECK(is1.ranges_[2] == range(2,3,4));
  }

  {
    boost::detail::multi_array::index_gen<3,2> is2 = 
      indices[range(0,1,2)][2][range(2,3,4)];
    BOOST_CHECK(is2.ranges_[0] == range(0,1,2));
    BOOST_CHECK(is2.ranges_[1] == range(2));
    BOOST_CHECK(is2.ranges_[1].is_degenerate());
    BOOST_CHECK(is2.ranges_[2] == range(2,3,4));
  }

  return boost::exit_success;
}

