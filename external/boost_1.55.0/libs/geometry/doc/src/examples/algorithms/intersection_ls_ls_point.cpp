// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[intersection_ls_ls_point
//` Calculate the intersection of two linestrings

#include <iostream>
#include <deque>

#include <boost/geometry.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

#include <boost/foreach.hpp>

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)


int main()
{
    typedef boost::geometry::model::d2::point_xy<double> P;
    std::vector<P> line1, line2;
    boost::geometry::read_wkt("linestring(1 1,2 2,3 1)", line1);
    boost::geometry::read_wkt("linestring(1 2,2 1,3 2)", line2);

    std::deque<P> intersection_points;
    boost::geometry::intersection(line1, line2, intersection_points);

    BOOST_FOREACH(P const& p, intersection_points)
    {
        std::cout << " " << boost::geometry::wkt(p);
    }
    std::cout << std::endl;

    return 0;
}

//]


//[intersection_ls_ls_point_output
/*`
Output:
[pre
 POINT(1.5 1.5) POINT(2.5 1.5)
]
*/
//]
