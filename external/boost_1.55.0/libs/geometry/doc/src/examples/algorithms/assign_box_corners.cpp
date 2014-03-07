// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[assign_box_corners
//` Shows how four point can be assigned from a 2D box

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

using namespace boost::geometry;

int main()
{
    typedef model::d2::point_xy<double> point;
    typedef model::box<point> box;

    box b;
    assign_values(b, 2, 2, 5, 5);

    point ll, lr, ul, ur;
    assign_box_corners(b, ll, lr, ul, ur);

    std::cout << "box: " << dsv(b) << std::endl << std::endl;

    std::cout << dsv(ul) << " --- " << dsv(ur) << std::endl;
    for (int i = 0; i < 3; i++)
    {
        std::cout << "  |          |" << std::endl;
    }
    std::cout << dsv(ll) << " --- " << dsv(lr) << std::endl;

    return 0;
}

//]


//[assign_box_corners_output
/*`
Output:
[pre
box: ((2, 2), (5, 5))

(2, 5) --- (5, 5)
  |          |
  |          |
  |          |
(2, 2) --- (5, 2)
]
*/
//]
