// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[point_order
//` Examine the expected point order of a polygon type

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::polygon<point_type, false> polygon_type;

    boost::geometry::order_selector order = boost::geometry::point_order<polygon_type>::value;
    
    std::cout << "order: " << order << std::endl
        << "(clockwise = " << boost::geometry::clockwise
        << ", counterclockwise = " << boost::geometry::counterclockwise
        << ") "<< std::endl;

    return 0;
}

//]


//[point_order_output
/*`
Output:
[pre
order: 2
(clockwise = 1, counterclockwise = 2)
]
*/
//]
