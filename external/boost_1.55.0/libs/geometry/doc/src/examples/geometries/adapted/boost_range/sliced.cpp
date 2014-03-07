// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[boost_range_sliced
//` Shows how to use a Boost.Geometry linestring, sliced by Boost.Range adaptor

#include <iostream>

#include <boost/assign.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_range/sliced.hpp>


int main()
{
    using namespace boost::assign;
    
    typedef boost::geometry::model::d2::point_xy<int> xy;
    boost::geometry::model::linestring<xy> line;
    line += xy(0, 0);
    line += xy(1, 1);
    line += xy(2, 2);
    line += xy(3, 3);
    line += xy(4, 4);
    
    std::cout 
        << boost::geometry::dsv(line | boost::adaptors::sliced(1, 3)) << std::endl;

    return 0;
}

//]

//[boost_range_sliced_output
/*`
Output:
[pre
((1, 1), (2, 2))
]
*/
//]
