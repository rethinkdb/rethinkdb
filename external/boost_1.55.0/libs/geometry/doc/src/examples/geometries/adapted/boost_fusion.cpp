// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[boost_fusion
//` Shows how to combine Boost.Fusion with Boost.Geometry

#include <iostream>

#include <boost/fusion/include/adapt_struct_named.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_fusion.hpp>


struct sample_point
{
    double x, y, z;
};

BOOST_FUSION_ADAPT_STRUCT(sample_point, (double, x) (double, y) (double, z))
BOOST_GEOMETRY_REGISTER_BOOST_FUSION_CS(cs::cartesian)

int main()
{
    sample_point a, b, c;
    
    // Set coordinates the Boost.Geometry way (one of the ways)
    boost::geometry::assign_values(a, 3, 2, 1);
    
    // Set coordinates the Boost.Fusion way
    boost::fusion::at_c<0>(b) = 6;
    boost::fusion::at_c<1>(b) = 5;
    boost::fusion::at_c<2>(b) = 4;
    
    // Set coordinates the native way
    c.x = 9;
    c.y = 8;
    c.z = 7;
    
    std::cout << "Distance a-b: " << boost::geometry::distance(a, b) << std::endl;
    std::cout << "Distance a-c: " << boost::geometry::distance(a, c) << std::endl;

    return 0;
}

//]

//[boost_fusion_output
/*`
Output:
[pre
Distance a-b: 5.19615
Distance a-c: 10.3923
]
*/
//]
