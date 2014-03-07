// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_unique.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>



template <typename Point>
void test_all()
{
    test_geometry<bg::model::linestring<Point> >(
        "LINESTRING(0 0,1 1)",
        "LINESTRING(0 0,1 1)");

    test_geometry<bg::model::linestring<Point> >(
        "LINESTRING(0 0,1 1,1 1)",
        "LINESTRING(0 0,1 1)");

    test_geometry<bg::model::linestring<Point> >(
        "LINESTRING(0 0,0 0,1 1)",
        "LINESTRING(0 0,1 1)");

    // Consecutive points
    test_geometry<bg::model::linestring<Point> >(
        "LINESTRING(0 0,0 0,0 0,0 0,1 1,1 1,1 1)",
        "LINESTRING(0 0,1 1)");

    // Other types
    test_geometry<bg::model::ring<Point> >(
        "POLYGON((0 0,0 1,1 1,1 1,1 1,1 0,0 0,0 0))",
        "POLYGON((0 0,0 1,1 1,1 0,0 0))");

    // With holes
    test_geometry<bg::model::polygon<Point> >(
        "POLYGON((0 0,0 10,10 10,10 10,10 10,10 0,0 0,0 0))",
        "POLYGON((0 0,0 10,10 10,10 0,0 0))");
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
