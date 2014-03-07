// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_ring_templated
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_RING_TEMPLATED

#include <iostream>
#include <deque>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/ring.hpp>

// Adapt any deque to Boost.Geometry Ring Concept
BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(std::deque) 

int main()
{
    std::deque<boost::geometry::model::d2::point_xy<double> > ring(3);
    boost::geometry::assign_values(ring[0], 0, 0);
    boost::geometry::assign_values(ring[2], 4, 1);
    boost::geometry::assign_values(ring[1], 1, 4);
    
    // Boost.Geometry algorithms work on any deque now
    boost::geometry::correct(ring);
    std::cout << "Area: "  << boost::geometry::area(ring) << std::endl;
    std::cout << "Contents: "  << boost::geometry::wkt(ring) << std::endl;
    
    return 0;
}

//]


//[register_ring_templated_output
/*`
Output:
[pre
Area: 7.5
Line: ((0, 0), (1, 4), (4, 1), (0, 0))
]
*/
//]
