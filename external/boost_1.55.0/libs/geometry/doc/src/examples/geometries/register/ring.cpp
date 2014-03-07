// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_ring
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_RING

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/ring.hpp>

typedef boost::geometry::model::d2::point_xy<double> point_2d;

BOOST_GEOMETRY_REGISTER_RING(std::vector<point_2d>) /*< The magic: adapt vector to Boost.Geometry Ring Concept >*/

int main()
{
    // Normal usage of std::
    std::vector<point_2d> ring;
    ring.push_back(point_2d(1, 1));
    ring.push_back(point_2d(2, 2));
    ring.push_back(point_2d(2, 1));
    
    
    // Usage of Boost.Geometry
    boost::geometry::correct(ring);
    std::cout << "Area: "  << boost::geometry::area(ring) << std::endl;
    std::cout << "WKT: "  << boost::geometry::wkt(ring) << std::endl;
    
    return 0;
}

//]


//[register_ring_output
/*`
Output:
[pre
Area: 0.5
WKT: POLYGON((1 1,2 2,2 1,1 1))
]
*/
//]
