// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2011-2012 Akira Takahashi

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom point / Boost.Fusion Example

#include <iostream>

#include <boost/fusion/include/adapt_struct_named.hpp>

#include <boost/geometry/algorithms/distance.hpp>

#include <boost/geometry/geometries/adapted/boost_fusion.hpp>

#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/io/dsv/write.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_FUSION_CS(cs::cartesian);


// Sample point, having x/y
struct my_2d
{
    float x,y;
};


BOOST_FUSION_ADAPT_STRUCT(my_2d,
    (float, x)
    (float, y))


int main()
{
    my_2d p1 = {1, 5};
    my_2d p2 = {3, 4};

    std::cout << "Coordinate using direct access: "
        << p1.x
        << std::endl;
    std::cout << "Coordinate using Boost.Fusion: "
        << boost::fusion::at_c<0>(p1)
        << std::endl;
    std::cout << "Coordinate using Boost.Geometry: "
        << boost::geometry::get<0>(p1)
        << std::endl;

    std::cout << "Two points are: "
        << boost::geometry::dsv(p1) << " "
        << boost::geometry::dsv(p2) << std::endl;

    std::cout << "Distance: "
        << boost::geometry::distance(p1, p2)
        << std::endl;

    return 0;
}
