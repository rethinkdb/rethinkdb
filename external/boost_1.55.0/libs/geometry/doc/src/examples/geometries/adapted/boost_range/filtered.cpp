// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[boost_range_filtered
//` Shows how to use a Boost.Geometry linestring, filtered by Boost.Range adaptor

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_range/filtered.hpp>

struct not_two
{
    template <typename P>
    bool operator()(P const& p) const
    {
        return boost::geometry::get<1>(p) != 2;
    }
};


int main()
{
    typedef boost::geometry::model::d2::point_xy<int> xy;
    boost::geometry::model::linestring<xy> line;
    line.push_back(xy(0, 0));
    line.push_back(xy(1, 1));
    line.push_back(xy(2, 2));
    line.push_back(xy(3, 1));
    line.push_back(xy(4, 0));
    line.push_back(xy(5, 1));
    line.push_back(xy(6, 2));
    line.push_back(xy(7, 1));
    line.push_back(xy(8, 0));
    
    using boost::adaptors::filtered;
    std::cout 
        << boost::geometry::length(line) << std::endl
        << boost::geometry::length(line | filtered(not_two())) << std::endl
        << boost::geometry::dsv(line | filtered(not_two())) << std::endl;

    return 0;
}

//]

//[boost_range_filtered_output
/*`
Output:
[pre
11.3137
9.65685
((0, 0), (1, 1), (3, 1), (4, 0), (5, 1), (7, 1), (8, 0))
]
*/
//]
