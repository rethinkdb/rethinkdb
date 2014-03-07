// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[sym_difference
//` Shows how to calculate the symmetric difference (XOR) of two polygons

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/foreach.hpp>
/*<-*/ #include "create_svg_overlay.hpp" /*->*/

int main()
{
    typedef boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double> > polygon;

    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    boost::geometry::model::multi_polygon<polygon> multi;

    boost::geometry::sym_difference(green, blue, multi);

    std::cout
        << "green XOR blue:" << std::endl
        << "total: " << boost::geometry::area(multi) << std::endl;
    int i = 0;
    BOOST_FOREACH(polygon const& p, multi)
    {
        std::cout << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

    /*<-*/ create_svg("sym_difference.svg", green, blue, multi); /*->*/
    return 0;
}

//]


//[sym_difference_output
/*`
Output:
[pre
green XOR blue:
total: 3.1459
0: 0.02375
1: 0.542951
2: 0.0149697
3: 0.226855
4: 0.839424
5: 0.525154
6: 0.015
7: 0.181136
8: 0.128798
9: 0.340083
10: 0.307778

[$img/algorithms/sym_difference.png]

]
*/
//]

