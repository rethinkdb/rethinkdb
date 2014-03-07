// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[area
//` Calculate the area of a polygon

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int main()
{
    // Calculate the area of a cartesian polygon
    bg::model::polygon<bg::model::d2::point_xy<double> > poly;
    bg::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly);
    double area = bg::area(poly);
    std::cout << "Area: " << area << std::endl;

    // Calculate the area of a spherical equatorial polygon
    bg::model::polygon<bg::model::point<float, 2, bg::cs::spherical_equatorial<bg::degree> > > sph_poly;
    bg::read_wkt("POLYGON((0 0,0 45,45 0,0 0))", sph_poly);
    area = bg::area(sph_poly);
    std::cout << "Area: " << area << std::endl;

    return 0;
}

//]


//[area_output
/*`
Output:
[pre
Area: 16
Area: 0.339837
]
*/
//]
