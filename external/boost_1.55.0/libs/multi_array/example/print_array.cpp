// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.


#include <iostream>
#include "boost/multi_array.hpp"
#include "boost/array.hpp"
#include "boost/cstdlib.hpp"

template <typename Array>
void print(std::ostream& os, const Array& A)
{
  typename Array::const_iterator i;
  os << "[";
  for (i = A.begin(); i != A.end(); ++i) {
    print(os, *i);
    if (boost::next(i) != A.end())
      os << ',';
  }
  os << "]";
}
void print(std::ostream& os, const double& x)
{
  os << x;
}
int main()
{
  typedef   boost::multi_array<double, 2> array;
  double values[] = {
    0, 1, 2,
    3, 4, 5 
  };
  const int values_size=6;
  array A(boost::extents[2][3]);
  A.assign(values,values+values_size);
  print(std::cout, A);
  return boost::exit_success;
}

//  The output is: 
// [[0,1,2],[3,4,5]]
