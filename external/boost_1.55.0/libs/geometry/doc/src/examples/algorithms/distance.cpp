// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[distance
//` Shows calculation of distance of point to some other geometries

#include <iostream>
#include <list>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/foreach.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::polygon<point_type> polygon_type;
    typedef boost::geometry::model::linestring<point_type> linestring_type;
    typedef boost::geometry::model::multi_point<point_type> multi_point_type;

    point_type p(1,2);
    polygon_type poly;
    linestring_type line;
    multi_point_type mp;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);
    line.push_back(point_type(0,0));
    line.push_back(point_type(0,3));
    mp.push_back(point_type(0,0));
    mp.push_back(point_type(3,3));

    std::cout 
        << "Point-Poly: " << boost::geometry::distance(p, poly) << std::endl
        << "Point-Line: " << boost::geometry::distance(p, line) << std::endl
        << "Point-MultiPoint: " << boost::geometry::distance(p, mp) << std::endl;

    return 0;
}

//]


//[distance_output
/*`
Output:
[pre
Point-Poly: 1.22066
Point-Line: 1
Point-MultiPoint: 2.23607
]
*/
//]

