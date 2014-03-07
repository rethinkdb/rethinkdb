// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[transform_with_strategy
//` Shows how points can be scaled, translated or rotated

#include <iostream>
#include <boost/geometry.hpp>


int main()
{
    namespace trans = boost::geometry::strategy::transform;
    using boost::geometry::dsv;
    
    typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> point_type;

    point_type p1(1.0, 1.0);

    // Translate over (1.5, 1.5)
    point_type p2;
    trans::translate_transformer<double, 2, 2> translate(1.5, 1.5);
    boost::geometry::transform(p1, p2, translate);

    // Scale with factor 3.0
    point_type p3;
    trans::scale_transformer<double, 2, 2> scale(3.0);
    boost::geometry::transform(p1, p3, scale);

    // Rotate with respect to the origin (0,0) over 90 degrees (clockwise)
    point_type p4;
    trans::rotate_transformer<boost::geometry::degree, double, 2, 2> rotate(90.0);
    boost::geometry::transform(p1, p4, rotate);
    
    std::cout 
        << "p1: " << dsv(p1) << std::endl
        << "p2: " << dsv(p2) << std::endl
        << "p3: " << dsv(p3) << std::endl
        << "p4: " << dsv(p4) << std::endl;

    return 0;
}

//]


//[transform_with_strategy_output
/*`
Output:
[pre
p1: (1, 1)
p2: (2.5, 2.5)
p3: (3, 3)
p4: (1, -1)
]
*/
//]
