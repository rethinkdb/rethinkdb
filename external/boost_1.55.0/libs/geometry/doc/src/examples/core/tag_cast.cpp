// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[tag_cast
//` Check if the polygon_tag can be casted to the areal_tag

#include <iostream>
#include <typeinfo>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace geo = boost::geometry;
int main()
{
    typedef geo::model::d2::point_xy<double> point_type;
    typedef geo::model::polygon<point_type> polygon_type;

    typedef geo::tag<polygon_type>::type tag;
    typedef geo::tag_cast<tag, geo::linear_tag, geo::areal_tag>::type base_tag;
    
    std::cout << "tag: " << typeid(tag).name() << std::endl
        << "base tag: " << typeid(base_tag).name() << std::endl;

    return 0;
}

//]


//[tag_cast_output
/*`
Output (in MSVC):
[pre
tag: struct boost::geometry::polygon_tag
base tag: struct boost::geometry::areal_tag
]
*/
//]
