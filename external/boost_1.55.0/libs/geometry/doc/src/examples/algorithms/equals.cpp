// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[equals
//` Shows the predicate equals, which returns true if two geometries are spatially equal

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

#include <boost/assign.hpp>

int main()
{
    using boost::assign::tuple_list_of;

    typedef boost::tuple<int, int> point;

    boost::geometry::model::polygon<point> poly1, poly2;
    boost::geometry::exterior_ring(poly1) = tuple_list_of(0, 0)(0, 5)(5, 5)(5, 0)(0, 0);
    boost::geometry::exterior_ring(poly2) = tuple_list_of(5, 0)(0, 0)(0, 5)(5, 5)(5, 0);

    std::cout 
        << "polygons are spatially " 
        << (boost::geometry::equals(poly1, poly2) ? "equal" : "not equal")
        << std::endl;
    
    boost::geometry::model::box<point> box;
    boost::geometry::assign_values(box, 0, 0, 5, 5);
    
    std::cout 
        << "polygon and box are spatially " 
        << (boost::geometry::equals(box, poly2) ? "equal" : "not equal")
        << std::endl;
    

    return 0;
}

//]


//[equals_output
/*`
Output:
[pre
polygons are spatially equal
polygon and box are spatially equal
]
*/
//]
