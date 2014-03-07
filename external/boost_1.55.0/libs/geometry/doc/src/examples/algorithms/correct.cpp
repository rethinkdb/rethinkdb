// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[correct
//` Shows how to correct a polygon with respect to its orientation and closure

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

#include <boost/assign.hpp>

int main()
{
    using boost::assign::tuple_list_of;

    typedef boost::geometry::model::polygon
        <
            boost::tuple<int, int>
        > clockwise_closed_polygon;

    clockwise_closed_polygon cwcp;

    // Fill it counterclockwise (so wrongly), forgetting the closing point
    boost::geometry::exterior_ring(cwcp) = tuple_list_of(0, 0)(10, 10)(0, 9);

    // Add a counterclockwise closed inner ring (this is correct)
    boost::geometry::interior_rings(cwcp).push_back(tuple_list_of(1, 2)(4, 6)(2, 8)(1, 2));

    // Its area should be negative (because of wrong orientation)
    //     and wrong (because of omitted closing point)
    double area_before = boost::geometry::area(cwcp);

    // Correct it!
    boost::geometry::correct(cwcp);

    // Check its new area
    double area_after = boost::geometry::area(cwcp);

    // And output it
    std::cout << boost::geometry::dsv(cwcp) << std::endl;
    std::cout << area_before << " -> " << area_after << std::endl;

    return 0;
}

//]


//[correct_output
/*`
Output:
[pre
(((0, 0), (0, 9), (10, 10), (0, 0)), ((1, 2), (4, 6), (2, 8), (1, 2)))
-7 -> 38
]
*/
//]
