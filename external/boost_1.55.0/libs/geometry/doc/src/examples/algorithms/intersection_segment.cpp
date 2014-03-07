// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[intersection_segment
//` Calculate the intersection point (or points) of two segments

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/foreach.hpp>


int main()
{
    typedef boost::geometry::model::d2::point_xy<double> P;
    boost::geometry::model::segment<P> segment1, segment2;
    boost::geometry::read_wkt("linestring(1 1,2 2)", segment1);
    boost::geometry::read_wkt("linestring(2 1,1 2)", segment2);

    std::vector<P> intersections;
    boost::geometry::intersection(segment1, segment2, intersections);

    BOOST_FOREACH(P const& p, intersections)
    {
        std::cout << " " << boost::geometry::wkt(p);
    }
    std::cout << std::endl;

    return 0;
}

//]


//[intersection_segment_output
/*`
Output:
[pre
 POINT(1.5 1.5)
]
*/
//]
