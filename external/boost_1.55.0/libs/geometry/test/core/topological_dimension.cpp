// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>

#include <boost/geometry/core/topological_dimension.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>


#include <vector>
#include <deque>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)


template <typename G, int Expected>
void test_geometry()
{
    BOOST_CHECK_EQUAL(bg::topological_dimension<G>::type::value, Expected);
}

template <typename P>
void test_all()
{
    test_geometry<P, 0>();
    test_geometry<P const, 0>();
    test_geometry<bg::model::linestring<P> , 1>();
    test_geometry<bg::model::ring<P> , 2>(); // being discussed
    test_geometry<bg::model::polygon<P> , 2>();
    test_geometry<bg::model::box<P> , 2>();
    test_geometry<bg::model::segment<P> , 1>();
    test_geometry<bg::model::referring_segment<P const> , 1>();

    test_geometry<std::vector<P>, 1>();
    test_geometry<std::deque<P>, 1>();

}

int test_main(int, char* [])
{
    test_geometry<int[2], 0>();
    test_geometry<float[2], 0>();
    test_geometry<double[2], 0>();

    test_geometry<int[3], 0>();
    test_geometry<float[3], 0>();
    test_geometry<double[3], 0>();

    test_geometry<boost::tuple<double, double>, 0>();
    test_geometry<boost::tuple<double, double, double>, 0>();

    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
