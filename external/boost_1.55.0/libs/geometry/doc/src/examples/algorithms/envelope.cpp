// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[envelope
//` Shows how to calculate the bounding box of a polygon

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

/*<-*/ #include "create_svg_two.hpp" /*->*/

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point;

    boost::geometry::model::polygon<point> polygon;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", polygon);

    boost::geometry::model::box<point> box;
    boost::geometry::envelope(polygon, box);

    std::cout << "envelope:" << boost::geometry::dsv(box) << std::endl;

    /*<-*/ create_svg("envelope.svg", polygon, box); /*->*/
    return 0;
}

//]


//[envelope_output
/*`
Output:
[pre
envelope:((2, 0.7), (5.4, 3))

[$img/algorithms/envelope.png]
]
*/
//]

