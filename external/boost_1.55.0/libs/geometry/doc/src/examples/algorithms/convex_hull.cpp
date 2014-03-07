// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[convex_hull
//` Shows how to generate the convex_hull of a geometry

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

/*<-*/ #include "create_svg_two.hpp" /*->*/
int main()
{
    typedef boost::tuple<double, double> point;
    typedef boost::geometry::model::polygon<point> polygon;

    polygon poly;
    boost::geometry::read_wkt("polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0"
        ", 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))", poly);

    polygon hull;
    boost::geometry::convex_hull(poly, hull);
        
    using boost::geometry::dsv;
    std::cout
        << "polygon: " << dsv(poly) << std::endl
        << "hull: " << dsv(hull) << std::endl
        ;

    /*<-*/ create_svg("hull.svg", poly, hull); /*->*/
    return 0;
}

//]


//[convex_hull_output
/*`
Output:
[pre
polygon: (((2, 1.3), (2.4, 1.7), (2.8, 1.8), (3.4, 1.2), (3.7, 1.6), (3.4, 2), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))
hull: (((2, 1.3), (2.4, 1.7), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))

[$img/algorithms/convex_hull.png]
]
*/
//]
