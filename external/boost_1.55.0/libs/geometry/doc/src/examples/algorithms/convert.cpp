// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[convert
//` Shows how to convert a geometry into another geometry

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point;
    typedef boost::geometry::model::box<point> box;
    typedef boost::geometry::model::polygon<point> polygon;

    point p1(1, 1);
    box bx = boost::geometry::make<box>(1, 1, 2, 2);
    
    // Assign a box to a polygon (conversion box->poly)
    polygon poly;
    boost::geometry::convert(bx, poly);

    // Convert a point to another point type (conversion of point-type)
    boost::tuple<double, double> p2;
    boost::geometry::convert(p1, p2); // source -> target
        
    using boost::geometry::dsv;
    std::cout
        << "box: " << dsv(bx) << std::endl
        << "polygon: " << dsv(poly) << std::endl
        << "point: " << dsv(p1) << std::endl
        << "point tuples: " << dsv(p2) << std::endl
        ;

    return 0;
}

//]


//[convert_output
/*`
Output:
[pre
box: ((1, 1), (2, 2))
polygon: (((1, 1), (1, 2), (2, 2), (2, 1), (1, 1)))
point: (1, 1)
point tuples: (1, 1)
]
*/
//]
