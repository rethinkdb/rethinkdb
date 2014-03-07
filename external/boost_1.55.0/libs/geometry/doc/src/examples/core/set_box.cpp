// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[set_box
//` Set the coordinate of a box

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace bg = boost::geometry;

int main()
{
    bg::model::box<bg::model::d2::point_xy<double> > box;

    bg::set<bg::min_corner, 0>(box, 0);
    bg::set<bg::min_corner, 1>(box, 2);
    bg::set<bg::max_corner, 0>(box, 4);
    bg::set<bg::max_corner, 1>(box, 5);

    std::cout << "Extent: " << bg::dsv(box) << std::endl;

    return 0;
}

//]


//[set_box_output
/*`
Output:
[pre
Extent: ((0, 2), (4, 5))
]
*/
//]
