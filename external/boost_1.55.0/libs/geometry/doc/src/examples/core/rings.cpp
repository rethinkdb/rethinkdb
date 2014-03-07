// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[rings
/*`
Shows how to access the exterior ring (one) 
and interior rings (zero or more) of a polygon.
Also shows the related ring_type and interior_type.
*/

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>


int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point;
    typedef boost::geometry::model::polygon<point> polygon_type;

    polygon_type poly;
    
    typedef boost::geometry::ring_type<polygon_type>::type ring_type;
    ring_type& ring = boost::geometry::exterior_ring(poly);
    
    // For a ring of model::polygon, you can call "push_back".
    // (internally, it is done using a traits::push_back class)
    ring.push_back(point(0, 0));
    ring.push_back(point(0, 5));
    ring.push_back(point(5, 4));
    ring.push_back(point(0, 0));
    
    ring_type inner;
    inner.push_back(point(1, 1));
    inner.push_back(point(2, 1));
    inner.push_back(point(2, 2));
    inner.push_back(point(1, 1));

    typedef boost::geometry::interior_type<polygon_type>::type int_type;
    int_type& interiors = boost::geometry::interior_rings(poly);
    interiors.push_back(inner);

    std::cout << boost::geometry::dsv(poly) << std::endl;
    
    // So int_type defines a collection of rings, 
    // which is a Boost.Range compatible range
    // The type of an element of the collection is the very same ring type again.
    // We show that.
    typedef boost::range_value<int_type>::type int_ring_type;
    
    std::cout 
        << std::boolalpha
        << boost::is_same<ring_type, int_ring_type>::value 
        << std::endl;

    return 0;
}

//]

//[rings_output
/*`
Output:
[pre
(((0, 0), (0, 5), (5, 4), (0, 0)), ((1, 1), (2, 1), (2, 2), (1, 1)))
true
]
*/
//]
