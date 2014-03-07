// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_box_2d_4values
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES

#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>

struct my_point
{
    int x, y;
};

struct my_box
{
    int left, top, right, bottom;
};

BOOST_GEOMETRY_REGISTER_POINT_2D(my_point, int, cs::cartesian, x, y)

// Register the box type, also notifying that it is based on "my_point"
// (even if it does not contain it)
BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(my_box, my_point, left, top, right, bottom)

int main()
{
    my_box b = boost::geometry::make<my_box>(0, 0, 2, 2);
    std::cout << "Area: "  << boost::geometry::area(b) << std::endl;
    return 0;
}

//]


//[register_box_2d_4values_output
/*`
Output:
[pre
Area: 4
]
*/
//]
