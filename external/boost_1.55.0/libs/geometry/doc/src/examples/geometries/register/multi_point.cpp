// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_multi_point
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_MULTI_POINT

#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/multi/geometries/register/multi_point.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

typedef boost::tuple<float, float> point_type;

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_MULTI_POINT(std::deque< ::point_type >)

int main()
{
    // Normal usage of std::
    std::deque<point_type> multi_point;
    multi_point.push_back(point_type(1, 1));
    multi_point.push_back(point_type(3, 2));
    
    // Usage of Boost.Geometry
    std::cout << "WKT: "  << boost::geometry::wkt(multi_point) << std::endl;
    
    return 0;
}

//]


//[register_multi_point_output
/*`
Output:
[pre
WKT: MULTIPOINT((1 1),(3 2))
]
*/
//]
