// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[point_type
//` Examine the point type of a multi_polygon

#include <iostream>
#include <typeinfo>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::polygon<point_type> polygon_type;
    typedef boost::geometry::model::multi_polygon<polygon_type> mp_type;

    typedef boost::geometry::point_type<mp_type>::type ptype;

    std::cout << "point type: " << typeid(ptype).name() << std::endl;

    return 0;
}

//]


//[point_type_output
/*`
Output (in MSVC):
[pre
point type: class boost::geometry::model::d2::point_xy<double,struct boost::geometry::cs::cartesian>
]
*/
//]
