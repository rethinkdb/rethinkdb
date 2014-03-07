// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>


#include <boost/geometry/core/coordinate_type.hpp>


#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <boost/geometry/geometries/register/linestring.hpp>

#include <vector>
#include <deque>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)


template <typename G, typename Expected>
void test_geometry()
{
    BOOST_CHECK_EQUAL(typeid(typename bg::coordinate_type<G>::type).name(),
        typeid(Expected).name());
}

template <typename P, typename Expected>
void test_all()
{
    test_geometry<P, Expected>();
    test_geometry<P const, Expected>();
    test_geometry<bg::model::linestring<P> , Expected>();
    test_geometry<bg::model::ring<P> , Expected>();
    test_geometry<bg::model::polygon<P> , Expected>();
    test_geometry<bg::model::box<P> , Expected>();
    test_geometry<bg::model::segment<P> , Expected>();
    test_geometry<bg::model::referring_segment<P const> , Expected>();

    test_geometry<std::vector<P>, Expected>();
    test_geometry<std::deque<P>, Expected>();
}

int test_main(int, char* [])
{
    test_geometry<int[2], int>();
    test_geometry<float[2], float>();
    test_geometry<double[2], double>();

    test_geometry<int[3], int>();
    test_geometry<float[3], float>();
    test_geometry<double[3], double>();

    test_geometry<boost::tuple<float, float>, float>();
    test_geometry<boost::tuple<double, double>, double>();
    test_geometry<boost::tuple<long double, long double>, long double>();
    test_geometry<boost::tuple<double, double, double>, double>();

    test_all<bg::model::point<int, 2, bg::cs::cartesian>, int>();
    test_all<bg::model::point<float, 2, bg::cs::cartesian>, float>();
    test_all<bg::model::point<double, 2, bg::cs::cartesian>, double>();
    test_all<bg::model::point<long double, 2, bg::cs::cartesian>, long double>();

    return 0;
}
