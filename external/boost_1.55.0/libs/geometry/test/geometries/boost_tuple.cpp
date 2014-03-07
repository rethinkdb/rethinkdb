// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include <iostream>


BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P>
void test_all()
{
    P p1, p2;
    bg::distance(p1, p2);
}

int test_main(int, char* [])
{
    test_all<boost::tuple<float> >();
    test_all<boost::tuple<int, int> >();
    test_all<boost::tuple<double, double, double> >();
    test_all<boost::tuple<float, float, float, float> >();
    test_all<boost::tuple<float, float, float, float, float> >();

    return 0;
}
