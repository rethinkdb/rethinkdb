// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[make_2d_point
//` Shows the usage of make as a generic constructor for different point types

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon/point.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

struct mypoint { float _x, _y; };

BOOST_GEOMETRY_REGISTER_POINT_2D(mypoint, float, cs::cartesian, _x, _y)

template <typename Point>
void construct_and_display()
{
    using boost::geometry::make;
    using boost::geometry::get;

    Point p = make<Point>(1, 2);

    std::cout << "x=" << get<0>(p) << " y=" << get<1>(p)
        << " (" << typeid(Point).name() << ")"
        << std::endl;
}

int main()
{
    construct_and_display<boost::geometry::model::d2::point_xy<double> >();
    construct_and_display<boost::geometry::model::d2::point_xy<int> >();
    construct_and_display<boost::tuple<double, double> >();
    construct_and_display<boost::polygon::point_data<int> >();
    construct_and_display<mypoint>();
    return 0;
}

//]

//[make_2d_point_output
/*`
Output (compiled using gcc):
[pre
x=1 y=2 (N5boost8geometry5model2d28point_xyIdNS0_2cs9cartesianEEE)
x=1 y=2 (N5boost8geometry5model2d28point_xyIiNS0_2cs9cartesianEEE)
x=1 y=2 (N5boost6tuples5tupleIddNS0_9null_typeES2_S2_S2_S2_S2_S2_S2_EE)
x=1 y=2 (N5boost7polygon10point_dataIiEE)
x=1 y=2 (7mypoint)
]
*/
//]
