// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[num_points
//` Get the number of points in a geometry

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/multi/multi.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>


int main()
{
    boost::geometry::model::multi_polygon
        <
            boost::geometry::model::polygon
                <
                    boost::geometry::model::d2::point_xy<double>
                >
        > mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 0,0 10,10 0,0 0),(1 1,1 9,9 1,1 1)),((10 10,10 7,7 10,10 10)))", mp);
    std::cout << "Number of points: " << boost::geometry::num_points(mp) << std::endl;

    return 0;
}

//]


//[num_points_output
/*`
Output:
[pre
 Number of points: 12
]
*/
//]
