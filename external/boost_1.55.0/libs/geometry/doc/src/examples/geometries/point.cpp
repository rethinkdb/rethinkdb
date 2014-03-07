// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[point
//` Declaration and use of the Boost.Geometry model::point, modelling the Point Concept

#include <iostream>
#include <boost/geometry.hpp>

namespace bg = boost::geometry;

int main()
{
    bg::model::point<double, 2, bg::cs::cartesian> point1;
    bg::model::point<double, 3, bg::cs::cartesian> point2(1.0, 2.0, 3.0); /*< Construct, assigning three coordinates >*/
    point1.set<0>(1.0); /*< Set a coordinate. [*Note]: prefer using `bg::set<0>(point1, 1.0);` >*/
    point1.set<1>(2.0);

    double x = point1.get<0>(); /*< Get a coordinate. [*Note]: prefer using `x = bg::get<0>(point1);` >*/
    double y = point1.get<1>();

    std::cout << x << ", " << y << std::endl;
    return 0;
}

//]


//[point_output
/*`
Output:
[pre
1, 2
]
*/
//]
