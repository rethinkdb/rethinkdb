// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[closure
//` Examine if a polygon is defined as "should be closed"

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::polygon<point_type> polygon_type;

    boost::geometry::closure_selector clos = boost::geometry::closure<polygon_type>::value;
    
    std::cout << "closure: " << clos << std::endl
        << "(open = " << boost::geometry::open
        << ", closed = " << boost::geometry::closed 
        << ") "<< std::endl;

    return 0;
}

//]


//[closure_output
/*`
Output:
[pre
closure: 1
(open = 0, closed = 1)
]
*/
//]
