// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[boost_range_uniqued
//` Shows how to use a Boost.Geometry ring, made unique by Boost.Range adaptor

#include <iostream>

#include <boost/assign.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/adapted/boost_range/uniqued.hpp>

typedef boost::geometry::model::d2::point_xy<int> xy;

inline bool operator==(xy const& left, xy const& right)
{
    boost::geometry::equal_to<xy> eq;
    return eq(left, right);
}


int main()
{
    using namespace boost::assign;
    using boost::adaptors::uniqued;
    
    boost::geometry::model::ring<xy> ring;
    ring += xy(0, 0);
    ring += xy(0, 1);
    ring += xy(0, 2);
    ring += xy(1, 2);
    ring += xy(2, 2);
    ring += xy(2, 2);
    ring += xy(2, 2);
    ring += xy(2, 0);
    ring += xy(0, 0);
    
    std::cout 
        << "Normal: " << boost::geometry::dsv(ring) << std::endl
        << "Unique: " << boost::geometry::dsv(ring | uniqued) << std::endl;

    return 0;
}

//]

//[boost_range_uniqued_output
/*`
Output:
[pre
Normal : ((0, 0), (0, 1), (0, 2), (1, 2), (2, 2), (2, 0), (0, 0))
uniqued: ((0, 0), (0, 2), (2, 2), (0, 0))
]
*/
//]
