// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[coordinate_type
//` Examine the coordinate type of a point

#include <iostream>
#include <typeinfo>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::polygon<point_type> polygon_type;

    typedef boost::geometry::coordinate_type<polygon_type>::type ctype;
    
    std::cout << "type: " << typeid(ctype).name() << std::endl;

    return 0;
}

//]


//[coordinate_type_output
/*`
Output (using MSVC):
[pre
type: double
]
*/
//]
