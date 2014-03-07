// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[c_array
//` Small example showing the combination of an array with a Boost.Geometry algorithm

#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian) 

int main()
{
    int a[3] = {1, 2, 3};
    int b[3] = {2, 3, 4};

    std::cout << boost::geometry::distance(a, b) << std::endl;
    
    return 0;
}

//]

//[c_array_output
/*`
Output:
[pre
1.73205
]
*/
//]
