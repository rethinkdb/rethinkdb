// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_linestring
//` Show the use of BOOST_GEOMETRY_REGISTER_LINESTRING

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

typedef boost::geometry::model::d2::point_xy<double> point_2d;

BOOST_GEOMETRY_REGISTER_LINESTRING(std::vector<point_2d>) 

int main()
{
    // Normal usage of std::
    std::vector<point_2d> line;
    line.push_back(point_2d(1, 1));
    line.push_back(point_2d(2, 2));
    line.push_back(point_2d(3, 1));
    
    // Usage of Boost.Geometry's length and wkt functions
    std::cout << "Length: "  
        << boost::geometry::length(line)
        << std::endl;
        
    std::cout << "WKT: "  
        << boost::geometry::wkt(line)
        << std::endl;
        
    return 0;
}

//]


//[register_linestring_output
/*`
Output:
[pre
Length: 2.82843
WKT: LINESTRING(1 1,2 2,3 1)
]
*/
//]
