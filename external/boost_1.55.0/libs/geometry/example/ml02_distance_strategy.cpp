// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Multipolygon DP simplification example from the mailing list discussion
// about the DP algorithm issue:
// http://lists.osgeo.org/pipermail/ggl/2011-September/001533.html

#include <boost/geometry.hpp>
#include <boost/geometry/strategies/cartesian/distance_pythagoras.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
using namespace boost::geometry;

int main()
{
  typedef model::d2::point_xy<double> point_xy;

  point_xy p1(0.0, 0.0);
  point_xy p2(5.0, 0.0);

  // 1) This is direct call to Pythagoras algo
  typedef strategy::distance::pythagoras<point_xy, point_xy, double> strategy1_type;
  strategy1_type strategy1;
  strategy1_type ::calculation_type d1 = strategy1.apply(p1, p2);

  // 2) This is what is effectively called by simplify
  typedef strategy::distance::comparable::pythagoras<point_xy, point_xy, double> strategy2_type;
  strategy2_type strategy2;
  strategy2_type::calculation_type d2 = strategy2.apply(p1, p2);

  return 0;  
}
