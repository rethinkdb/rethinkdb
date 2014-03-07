// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[register_multi_linestring
//` Show the use of the macro BOOST_GEOMETRY_REGISTER_MULTI_LINESTRING

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/multi/geometries/register/multi_linestring.hpp>

typedef boost::geometry::model::linestring
    <
        boost::tuple<float, float> 
    > linestring_type;

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_MULTI_LINESTRING(std::deque<linestring_type>)

int main()
{
    // Normal usage of std::
    std::deque<linestring_type> lines(2);
    boost::geometry::read_wkt("LINESTRING(0 0,1 1)", lines[0]);
    boost::geometry::read_wkt("LINESTRING(2 2,3 3)", lines[1]);
    
    // Usage of Boost.Geometry
    std::cout << "LENGTH: "  << boost::geometry::length(lines) << std::endl;
    
    return 0;
}

//]


//[register_multi_linestring_output
/*`
Output:
[pre
LENGTH: 2.82843
]
*/
//]
