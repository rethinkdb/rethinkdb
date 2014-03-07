// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[boost_array
//` Shows how to use a Boost.Array using Boost.Geometry's distance, set and assign_values algorithms

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/adapted/boost_array.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(cs::cartesian)

int main()
{
    boost::array<float, 2> a = { {1, 2} };
    boost::array<double, 2> b = { {2, 3} };
    std::cout << boost::geometry::distance(a, b) << std::endl;
    
    boost::geometry::set<0>(a, 1.1);
    boost::geometry::set<1>(a, 2.2);
    std::cout << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::assign_values(b, 2.2, 3.3);
    std::cout << boost::geometry::distance(a, b) << std::endl;
    
    boost::geometry::model::linestring<boost::array<double, 2> > line;
    line.push_back(b);

    return 0;
}

//]

//[boost_array_output
/*`
Output:
[pre
1.41421
1.20416
1.55563
]
*/
//]
