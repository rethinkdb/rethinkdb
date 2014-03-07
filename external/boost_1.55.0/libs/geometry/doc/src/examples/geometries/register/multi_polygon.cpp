// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_multi_polygon
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_MULTI_POLYGON

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/multi/geometries/register/multi_polygon.hpp>

typedef boost::geometry::model::polygon
    <
        boost::tuple<float, float> 
    > polygon_type;

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_MULTI_POLYGON(std::vector<polygon_type>)

int main()
{
    // Normal usage of std::
    std::vector<polygon_type> polygons(2);
    boost::geometry::read_wkt("POLYGON((0 0,0 1,1 1,1 0,0 0))", polygons[0]);
    boost::geometry::read_wkt("POLYGON((3 0,3 1,4 1,4 0,3 0))", polygons[1]);
    
    // Usage of Boost.Geometry
    std::cout << "AREA: "  << boost::geometry::area(polygons) << std::endl;
    
    return 0;
}

//]


//[register_multi_polygon_output
/*`
Output:
[pre
AREA: 2
]
*/
//]
