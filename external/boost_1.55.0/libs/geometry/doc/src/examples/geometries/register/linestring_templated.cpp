// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_linestring_templated
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED

#include <iostream>
#include <deque>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

// Adapt any deque to Boost.Geometry Linestring Concept
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque) 

int main()
{
    std::deque<boost::geometry::model::d2::point_xy<double> > line(2);
    boost::geometry::assign_values(line[0], 1, 1);
    boost::geometry::assign_values(line[1], 2, 2);
    
    // Boost.Geometry algorithms work on any deque now
    std::cout << "Length: "  << boost::geometry::length(line) << std::endl;
    std::cout << "Line: "  << boost::geometry::dsv(line) << std::endl;
    
    return 0;
}

//]


//[register_linestring_templated_output
/*`
Output:
[pre
Length: 1.41421
Line: ((1, 1), (2, 2))
]
*/
//]
