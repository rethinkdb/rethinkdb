// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_reverse.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <test_common/test_point.hpp>
#include <test_geometries/all_custom_linestring.hpp>
#include <test_geometries/all_custom_ring.hpp>
#include <test_geometries/wrapped_boost_array.hpp>

template <typename LineString>
void test_linestring()
{
    // Simplex
    test_geometry<LineString >(
        "LINESTRING(0 0,1 1)",
        "LINESTRING(1 1,0 0)");

    // Three points, middle should stay the same
    test_geometry<LineString >(
        "LINESTRING(0 0,1 1,2 2)",
        "LINESTRING(2 2,1 1,0 0)");

    // Four points
    test_geometry<LineString >(
        "LINESTRING(0 0,1 1,2 2,3 3)",
        "LINESTRING(3 3,2 2,1 1,0 0)");
}

template <typename Ring>
void test_ring()
{
    test_geometry<Ring>(
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0))",
        "POLYGON((4 0,2 1,0 2,0 7,4 9,8 7,8 2,4 0))");
}

template <typename Point>
void test_all()
{
    test_linestring<bg::model::linestring<Point> >();
    test_linestring<all_custom_linestring<Point> >();

    // Polygon with holes
    test_geometry<bg::model::polygon<Point> >(
        "POLYGON((4 0,8 2,8 7,4 9,0 7,0 2,2 1,4 0),(7 3,7 6,1 6,1 3,4 3,7 3))",
        "POLYGON((4 0,2 1,0 2,0 7,4 9,8 7,8 2,4 0),(7 3,4 3,1 3,1 6,7 6,7 3))");

    // Check compilation
    test_geometry<Point>("POINT(0 0)", "POINT(0 0)");

    test_ring<bg::model::ring<Point> >();
    test_ring<all_custom_ring<Point> >();
}

int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
