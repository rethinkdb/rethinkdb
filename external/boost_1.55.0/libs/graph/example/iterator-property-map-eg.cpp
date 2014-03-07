//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <iostream>
#include <boost/property_map/property_map.hpp>

int
main()
{
  using namespace boost;
  double x[] = { 0.2, 4.5, 3.2 };
  iterator_property_map < double *, identity_property_map, double, double& > pmap(x);
  std::cout << "x[1] = " << get(pmap, 1) << std::endl;
  put(pmap, 0, 1.7);
  std::cout << "x[0] = " << pmap[0] << std::endl;
  return 0;
}
