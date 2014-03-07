// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[assign_point_from_index
//` Shows how to retrieve one point from a box (either lower-left or upper-right) or segment

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/segment.hpp>

using namespace boost::geometry;

int main()
{
    typedef model::d2::point_xy<double> point;
    typedef model::segment<point> segment;

    segment s;
    assign_values(s, 1, 1, 2, 2);

    point first, second;
    assign_point_from_index<0>(s, first);
    assign_point_from_index<1>(s, second);
    std::cout
        << "segment: " << dsv(s) << std::endl
        << "first: " << dsv(first) << std::endl
        << "second: " << dsv(second) << std::endl;

    return 0;
}

//]


//[assign_point_from_index_output
/*`
Output:
[pre
segment: ((1, 1), (2, 2))
first: (1, 1)
second: (2, 2)
]
*/
//]
