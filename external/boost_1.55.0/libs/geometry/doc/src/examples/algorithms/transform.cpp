// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[transform
//` Shows how points can be transformed using the default strategy

#include <iostream>
#include <boost/geometry.hpp>


int main()
{
    namespace bg = boost::geometry;

    // Select a point near the pole (theta=5.0, phi=15.0)
    bg::model::point<long double, 2, bg::cs::spherical<bg::degree> > p1(15.0, 5.0);
    
    // Transform from degree to radian. Default strategy is automatically selected,
    // it will convert from degree to radian
    bg::model::point<long double, 2, bg::cs::spherical<bg::radian> > p2;
    bg::transform(p1, p2);
    
    // Transform from degree (lon-lat) to 3D (x,y,z). Default strategy is automatically selected, 
    // it will consider points on a unit sphere
    bg::model::point<long double, 3, bg::cs::cartesian> p3;
    bg::transform(p1, p3);
    
    std::cout 
        << "p1: " << bg::dsv(p1) << std::endl
        << "p2: " << bg::dsv(p2) << std::endl
        << "p3: " << bg::dsv(p3) << std::endl;

    return 0;
}

//]


//[transform_output
/*`
Output:
[pre
p1: (15, 5)
p2: (0.261799, 0.0872665)
p3: (0.084186, 0.0225576, 0.996195)
]
*/
//]
