// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>


#include <boost/geometry/core/tag.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/variant.hpp>

#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <boost/geometry/geometries/register/linestring.hpp>

#include <boost/variant/variant.hpp>

#include <vector>
#include <deque>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)


template <typename G, typename Expected>
void test_geometry()
{
    BOOST_CHECK_EQUAL(typeid(typename bg::point_type<G>::type).name(),
        typeid(Expected).name());
}

template <typename P>
void test_all()
{
    test_geometry<P, P>();
    test_geometry<P const, P>();
    test_geometry<bg::model::linestring<P> , P>();
    test_geometry<bg::model::ring<P> , P>();
    test_geometry<bg::model::polygon<P> , P>();
    test_geometry<bg::model::box<P> , P>();
    test_geometry<bg::model::segment<P> , P>();
    test_geometry<bg::model::referring_segment<P const> , P>();

    test_geometry<std::vector<P>, P>();
    test_geometry<std::deque<P>, P>();
}

int test_main(int, char* [])
{
    test_geometry<int[2], int[2]>();
    test_geometry<float[2], float[2]>();
    test_geometry<double[2], double[2]>();

    test_geometry<int[3], int[3]>();
    test_geometry<float[3], float[3]>();
    test_geometry<double[3], double[3]>();

    test_geometry<boost::tuple<double, double>,
                  boost::tuple<double, double> >();
    test_geometry<boost::tuple<double, double, double>,
                  boost::tuple<double, double, double> >();

    test_geometry<boost::variant<bg::model::box<boost::tuple<double, double> > >,
                  boost::tuple<double, double> >();

    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
