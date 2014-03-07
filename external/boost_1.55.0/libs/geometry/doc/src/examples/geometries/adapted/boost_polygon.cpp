// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[boost_polygon
//`Shows how to use Boost.Polygon points within Boost.Geometry

#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon.hpp>

int main()
{
    boost::polygon::point_data<int> a(1, 2), b(3, 4);
    std::cout << "Distance (using Boost.Geometry): " 
        << boost::geometry::distance(a, b) << std::endl;
    std::cout << "Distance (using Boost.Polygon): " 
        << boost::polygon::euclidean_distance(a, b) << std::endl;

    return 0;
}

//]

//[boost_polygon_output
/*`
Output:
[pre
Distance (using Boost.Geometry): 2.82843
Distance (using Boost.Polygon): 2.82843
]
*/
//]
