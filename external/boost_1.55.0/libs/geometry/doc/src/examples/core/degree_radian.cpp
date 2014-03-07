// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[degree_radian
//` Specify two coordinate systems, one in degrees, one in radians.

#include <iostream>
#include <boost/geometry.hpp>

using namespace boost::geometry;

int main()
{
    typedef model::point<double, 2, cs::spherical_equatorial<degree> > degree_point;
    typedef model::point<double, 2, cs::spherical_equatorial<radian> > radian_point;
    
    degree_point d(4.893, 52.373);
    radian_point r(0.041, 0.8527);

    double dist = distance(d, r);
    std::cout 
        << "distance:" << std::endl
        << dist << " over unit sphere" << std::endl
        << dist * 3959  << " over a spherical earth, in miles" << std::endl;

    return 0;
}

//]


//[degree_radian_output
/*`
Output:
[pre
distance:
0.0675272 over unit sphere
267.34 over a spherical earth, in miles
]
*/
//]
