// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

#include "boost/multi_array.hpp"
#include "for_each.hpp"
#include <algorithm>

struct times_five {
  double operator()(const int& val) { return val*5.0; }
};


int main() {

  typedef boost::multi_array<double,1> array;

  double data[] = {
    1.0, 2.0, 3.0,
    4.0, 5.0, 6.0,
    7.0, 8.0, 9.0
  };
  const int data_size=9;

  array A(boost::extents[9]);
  A.assign(data,data+data_size);

#if 0
  std::copy(A.begin(),A.end(),
            std::ostream_iterator<double>(std::cout,","));

  std::cout << "\n";
#endif
  
  for_each(A,times_five());

#if 0  
  std::copy(A.begin(),A.end(),
            std::ostream_iterator<double>(std::cout,","));
 
  std::cout << "\n";
#endif
  return 0;
}
