// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithms/test_convert.hpp>

#include <boost/geometry/multi/algorithms/convert.hpp>

#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>


template <typename Point1, typename Point2>
void test_mixed_point_types()
{
    test_mixed_identical_result
        <
            bg::model::multi_point<Point1>, 
            bg::model::multi_point<Point2> 
        >
        ("MULTIPOINT((1 1),(2 2),(3 3))");

    test_mixed_identical_result
        <
            bg::model::multi_linestring<bg::model::linestring<Point1> >, 
            bg::model::multi_linestring<bg::model::linestring<Point2> > 
        >
        ("MULTILINESTRING((1 1,2 2),(3 3,4 4))");

    // Single -> multi (always possible)
    test_mixed
        <
            Point1, bg::model::multi_point<Point2> 
        >
        (
            "POINT(1 1)", 
            "MULTIPOINT((1 1))"
        );
    test_mixed
        <
            bg::model::linestring<Point1>, 
            bg::model::multi_linestring<bg::model::linestring<Point2> > 
        >
        (
            "LINESTRING(1 1,2 2)", 
            "MULTILINESTRING((1 1,2 2))"
        );
    test_mixed
        <
            bg::model::segment<Point1>, 
            bg::model::multi_linestring<bg::model::linestring<Point2> > 
        >
        (
            "LINESTRING(1 1,2 2)", 
            "MULTILINESTRING((1 1,2 2))"
        );
    test_mixed
        <
            bg::model::box<Point1>, 
            bg::model::multi_polygon<bg::model::polygon<Point2> > 
        >
        (
            "BOX(0 0,1 1)", 
            "MULTIPOLYGON(((0 0,0 1,1 1,1 0,0 0)))"
        );
    test_mixed
        <
            bg::model::ring<Point1, true>, 
            bg::model::multi_polygon<bg::model::polygon<Point2, false> > 
        >
        (
            "POLYGON((0 0,0 1,1 1,1 0,0 0))", 
            "MULTIPOLYGON(((0 0,1 0,1 1,0 1,0 0)))"
        );

    // Multi -> single: should not compile (because multi often have 0 or >1 elements)
}

template <typename Point1, typename Point2>
void test_mixed_types()
{
    test_mixed_point_types<Point1, Point2>();
    test_mixed_point_types<Point2, Point1>();
}

int test_main( int , char* [] )
{
    test_mixed_types
        <
            bg::model::point<int, 2, bg::cs::cartesian>,
            bg::model::point<double, 2, bg::cs::cartesian>
        >();

    return 0;
}
