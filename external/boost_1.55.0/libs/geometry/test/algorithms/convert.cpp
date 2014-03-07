// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_convert.hpp>



template <typename Point1, typename Point2>
void test_mixed_point_types()
{
    // Point
    test_mixed_identical_result<Point1, Point2>("POINT(1 2)");

    // Box
    test_mixed_identical_result
        <
            bg::model::box<Point1>, 
            bg::model::box<Point2>
        >
        ("POLYGON((1 2,1 4,3 4,3 2,1 2))");

    test_mixed_identical_result
        <
            bg::model::segment<Point1>, 
            bg::model::segment<Point2>
        >
        ("LINESTRING(1 1,2 2)");

    // Linestring
    test_mixed_identical_result
        <
            bg::model::linestring<Point1>, 
            bg::model::linestring<Point2> 
        >
        ("LINESTRING(1 1,2 2)");

    // Ring
    test_mixed_identical_result
        <
            bg::model::ring<Point1>, 
            bg::model::ring<Point2> 
        >
        ("POLYGON((1 1,2 2,3 0,1 1))");

    test_mixed_reversible_result
        <
            bg::model::ring<Point1, true>, 
            bg::model::ring<Point2, false> 
        >
        (
            "POLYGON((1 1,2 2,3 0,1 1))", 
            "POLYGON((1 1,3 0,2 2,1 1))"
        );

    test_mixed
        <
            bg::model::ring<Point1, true, true>, 
            bg::model::ring<Point2, true, false> 
        >
        (
            "POLYGON((1 1,2 2,3 0,1 1))", 
            "POLYGON((1 1,2 2,3 0))"
        );

    test_mixed
        <
            bg::model::ring<Point1, true, false>, 
            bg::model::ring<Point2, true, true> 
        >
        (
            "POLYGON((1 1,2 2,3 0))", 
            "POLYGON((1 1,2 2,3 0,1 1))"
        );

    // Polygon
    test_mixed_reversible_result
        <
            bg::model::polygon<Point1, true>, 
            bg::model::polygon<Point2, false> 
        >
        (
            "POLYGON((0 0,0 5,5 5,5 0,0 0),(1 1,3 2,2 4,1 1))", 
            "POLYGON((0 0,5 0,5 5,0 5,0 0),(1 1,2 4,3 2,1 1))"
        );

    test_mixed
        <
            bg::model::polygon<Point1>,
            bg::model::polygon<Point2, false, false> 
        >
        (
            "POLYGON((0 0,0 5,5 5,5 0,0 0),(1 1,3 2,2 4,1 1))", 
            "POLYGON((0 0,5 0,5 5,0 5),(1 1,2 4,3 2))"
        );
    // (polygon uses ring, so other tests omitted here)

    // Combinations:
    // ring <-> polygon
    test_mixed_identical_result
        <
            bg::model::polygon<Point1>, 
            bg::model::ring<Point2> 
        >
        ("POLYGON((1 1,2 2,3 0,1 1))");

    test_mixed_reversible_result
        <
            bg::model::polygon<Point1, true>, 
            bg::model::ring<Point2, false> 
        >
        (   
            "POLYGON((1 1,2 2,3 0,1 1))", 
            "POLYGON((1 1,3 0,2 2,1 1))"
        );

    // Any hole will be omitted going from polygon to ring
    test_mixed
        <
            bg::model::polygon<Point1>, 
            bg::model::ring<Point2> 
        >
        (
            "POLYGON((0 0,0 5,5 5,5 0,0 0),(1 1,3 2,2 4,1 1))", 
            "POLYGON((0 0,0 5,5 5,5 0,0 0))"
        );

    // point -> box
    test_mixed
        <
            Point1, 
            bg::model::box<Point2> 
        >
        (
            "POINT(0 0)", 
            "POLYGON((0 0,0 0,0 0,0 0,0 0))"
        );

    // segment -> line
    test_mixed
        <
            bg::model::segment<Point1>,
            bg::model::linestring<Point2> 
        >
        (
            "LINESTRING(0 0,1 1)", 
            "LINESTRING(0 0,1 1)"
        );

    // box -> ring ( <- is NYI)
    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::ring<Point2> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,0 2,2 2,2 0,0 0))"
        );

    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::ring<Point2, false> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,2 0,2 2,0 2,0 0))"
        );

    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::ring<Point2, true, false> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,0 2,2 2,2 0))"
        );

    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::ring<Point2, false, false> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,2 0,2 2,0 2))"
        );

    // box -> polygon ( <- is NYI)
    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::polygon<Point2> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,0 2,2 2,2 0,0 0))"
        );

    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::polygon<Point2, false> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,2 0,2 2,0 2,0 0))"
        );

    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::polygon<Point2, true, false> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,0 2,2 2,2 0))"
        );

    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::polygon<Point2, false, false> 
        >
        (
            "BOX(0 0,2 2)", 
            "POLYGON((0 0,2 0,2 2,0 2))"
        );
}

template <typename Point1, typename Point2>
void test_mixed_point_types_3d()
{
    // Point
    test_mixed_identical_result<Point1, Point2>("POINT(1 2 3)");

    test_mixed_identical_result
        <
            bg::model::segment<Point1>, 
            bg::model::segment<Point2>
        >
        ("LINESTRING(1 2 3,4 5 6)");

    // Linestring
    test_mixed_identical_result
        <
            bg::model::linestring<Point1>, 
            bg::model::linestring<Point2> 
        >
        ("LINESTRING(1 2 3,4 5 6,7 8 9)");

    // segment -> line
    test_mixed
        <
            bg::model::segment<Point1>,
            bg::model::linestring<Point2> 
        >
        (
            "LINESTRING(1 2 3,4 5 6)", 
            "LINESTRING(1 2 3,4 5 6)"
        );
}



template <typename Point1, typename Point2>
void test_mixed_types()
{
    test_mixed_point_types<Point1, Point2>();
    test_mixed_point_types<Point2, Point1>();
}


template <typename Point1, typename Point2>
void test_mixed_types_3d()
{
    test_mixed_point_types_3d<Point1, Point2>();
    test_mixed_point_types_3d<Point2, Point1>();
}

void test_array()
{
    int a[2] = {1, 2};
    int b[2];
    bg::convert(a, b);
    BOOST_CHECK_EQUAL(b[0], 1);
    BOOST_CHECK_EQUAL(b[1], 2);
}

int test_main(int, char* [])
{
    test_mixed_types
        <
            bg::model::point<int, 2, bg::cs::cartesian>,
            bg::model::point<double, 2, bg::cs::cartesian>
        >();
    test_mixed_types
        <
            boost::tuple<float, float>,
            bg::model::point<float, 2, bg::cs::cartesian>
        >();
    test_mixed_types_3d
        <
            boost::tuple<double, double, double>,
            bg::model::point<double, 3, bg::cs::cartesian>
        >();

    test_array();
    return 0;
}
