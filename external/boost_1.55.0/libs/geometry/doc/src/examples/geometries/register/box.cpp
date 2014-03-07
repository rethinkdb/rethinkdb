// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_box
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_BOX

#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>

struct my_point
{
    double x, y;
};

struct my_box
{
    my_point ll, ur;
};

// Register the point type
BOOST_GEOMETRY_REGISTER_POINT_2D(my_point, double, cs::cartesian, x, y)

// Register the box type, also notifying that it is based on "my_point"
BOOST_GEOMETRY_REGISTER_BOX(my_box, my_point, ll, ur)

int main()
{
    my_box b = boost::geometry::make<my_box>(0, 0, 2, 2);
    std::cout << "Area: "  << boost::geometry::area(b) << std::endl;
    return 0;
}

//]


//[register_box_output
/*`
Output:
[pre
Area: 4
]
*/
//]
