// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[centroid
//` Shows calculation of a centroid of a polygon

#include <iostream>
#include <list>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
/*<-*/ #include "create_svg_two.hpp" /*->*/

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::polygon<point_type> polygon_type;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);

    point_type p;
    boost::geometry::centroid(poly, p);

    std::cout << "centroid: " << boost::geometry::dsv(p) << std::endl;
    /*<-*/ create_svg("centroid.svg", poly, p); /*->*/
    return 0;
}

//]

//[centroid_output
/*`
Output:
[pre
centroid: (4.04663, 1.6349)
[$img/algorithms/centroid.png]
]
Note that the centroid might be located in a hole or outside a polygon, easily.
*/
//]
