// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright Akira Takahashi 2011

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <boost/fusion/include/adapt_struct_named.hpp>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_fusion.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <iostream>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_FUSION_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


struct for_fusion_2d
{
    float x,y;
};
struct for_fusion_3d
{
    double x,y,z;
};

BOOST_FUSION_ADAPT_STRUCT(for_fusion_2d, (float, x) (float, y))
BOOST_FUSION_ADAPT_STRUCT(for_fusion_3d, (double, x) (double, y) (double, z))


void test_2d()
{
    bg::model::point<double, 2, bg::cs::cartesian> p1(1, 2);
    double p2[2] = {3, 4};
    boost::tuple<double, double> p3(5,6);

    for_fusion_2d pf = {7, 8};

    BOOST_CHECK_CLOSE(bg::distance(p1, pf), 8.4852813742385695, 0.01);
    BOOST_CHECK_CLOSE(bg::distance(p2, pf), 5.6568542494923806, 0.01);
    BOOST_CHECK_CLOSE(bg::distance(p3, pf), 2.82843, 0.01);
}

void test_3d()
{
    bg::model::point<double, 3, bg::cs::cartesian> p1(1, 2, 3);
    double p2[3] = {4, 5, 6};
    boost::tuple<double, double, double> p3(7, 8, 9);

    for_fusion_3d pf = {10, 11, 12};

    BOOST_CHECK_CLOSE(bg::distance(p1, pf), 15.58845726811, 0.01);
    BOOST_CHECK_CLOSE(bg::distance(p2, pf), 10.392304845413, 0.01);
    BOOST_CHECK_CLOSE(bg::distance(p3, pf), 5.196152, 0.01);
}

int test_main(int, char* [])
{
    test_2d();
    test_3d();
    return 0;
}

