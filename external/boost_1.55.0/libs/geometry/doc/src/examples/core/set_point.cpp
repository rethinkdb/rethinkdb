// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[set_point
//` Set the coordinate of a point

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace bg = boost::geometry;

int main()
{
    bg::model::d2::point_xy<double> point;

    bg::set<0>(point, 1);
    bg::set<1>(point, 2);

    std::cout << "Location: " << bg::dsv(point) << std::endl;

    return 0;
}

//]


//[set_point_output
/*`
Output:
[pre
Location: (1, 2)
]
*/
//]
