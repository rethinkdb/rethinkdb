// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

// #define BOOST_GEOMETRY_DEBUG_ASSEMBLE

#include <algorithms/test_intersection.hpp>
#include <algorithms/test_overlay.hpp>
#include <multi/algorithms/overlay/multi_overlay_cases.hpp>

#include <boost/geometry/multi/algorithms/correct.hpp>
#include <boost/geometry/multi/algorithms/intersection.hpp>
#include <boost/geometry/multi/algorithms/within.hpp> // only for testing #77

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/multi/io/wkt/read.hpp>

template <typename Ring, typename Polygon, typename MultiPolygon>
void test_areal()
{
    test_one<Polygon, MultiPolygon, MultiPolygon>("simplex_multi",
        case_multi_simplex[0], case_multi_simplex[1],
        2, 12, 6.42);

    test_one<Polygon, MultiPolygon, MultiPolygon>("case_multi_no_ip",
        case_multi_no_ip[0], case_multi_no_ip[1],
        2, 8, 8.5);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_multi_2",
        case_multi_2[0], case_multi_2[1],
        3, 12, 5.9);

    test_one<Polygon, MultiPolygon, Polygon>("simplex_multi_mp_p",
        case_multi_simplex[0], case_single_simplex,
        2, 12, 6.42);

    test_one<Polygon, Ring, MultiPolygon>("simplex_multi_r_mp",
        case_single_simplex, case_multi_simplex[0],
        2, 12, 6.42);
    test_one<Ring, MultiPolygon, Polygon>("simplex_multi_mp_r",
        case_multi_simplex[0], case_single_simplex,
        2, 12, 6.42);

    // Constructed cases for multi/touch/equal/etc
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_61_multi",
        case_61_multi[0], case_61_multi[1],
        0, 0, 0.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_62_multi",
        case_62_multi[0], case_62_multi[1],
        1, 5, 1.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_63_multi",
        case_63_multi[0], case_63_multi[1],
        1, 5, 1.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_64_multi",
        case_64_multi[0], case_64_multi[1],
        1, 5, 1.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_65_multi",
        case_65_multi[0], case_65_multi[1],
        1, 5, 1.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_72_multi",
        case_72_multi[0], case_72_multi[1],
        3, 14, 2.85);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_77_multi",
        case_77_multi[0], case_77_multi[1],
        5, 33, 9);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_78_multi",
        case_78_multi[0], case_78_multi[1],
        1, 0, 22); // In "get_turns" using partitioning, #points went from 17 to 16
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_101_multi",
        case_101_multi[0], case_101_multi[1],
        4, 22, 4.75);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_102_multi",
        case_102_multi[0], case_102_multi[1],
        3, 26, 19.75);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_107_multi",
        case_107_multi[0], case_107_multi[1],
        2, 10, 1.5);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_1",
        case_recursive_boxes_1[0], case_recursive_boxes_1[1],
        10, 97, 47.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_2",
        case_recursive_boxes_2[0], case_recursive_boxes_2[1],
        1, 47, 90.0); // Area from SQL Server

     test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_3",
         case_recursive_boxes_3[0], case_recursive_boxes_3[1],
         19, 87, 12.5); // Area from SQL Server

     test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_4",
         case_recursive_boxes_4[0], case_recursive_boxes_4[1],
         13, 157, 67.0); // Area from SQL Server

     test_one<Polygon, MultiPolygon, MultiPolygon>("ggl_list_20120915_h2_a",
         ggl_list_20120915_h2[0], ggl_list_20120915_h2[1],
         2, 10, 6.0); // Area from SQL Server
     test_one<Polygon, MultiPolygon, MultiPolygon>("ggl_list_20120915_h2_b",
         ggl_list_20120915_h2[0], ggl_list_20120915_h2[2],
         2, 10, 6.0); // Area from SQL Server
}

template <typename Polygon, typename MultiPolygon, typename Box>
void test_areal_clip()
{
    static std::string const clip = "POLYGON((1 1,4 4))";
    test_one<Polygon, Box, MultiPolygon>("simplex_multi_mp_b", clip, case_multi_simplex[0],
        2, 11, 6.791666);
    test_one<Polygon, MultiPolygon, Box>("simplex_multi_b_mp", case_multi_simplex[0], clip,
        2, 11, 6.791666);
}

template <typename LineString, typename MultiLineString, typename Box>
void test_linear()
{
    typedef typename bg::point_type<MultiLineString>::type point;
    test_one<point, MultiLineString, MultiLineString>("case_multi_ml_ml_1",
        "MULTILINESTRING((0 0,1 1))", "MULTILINESTRING((0 1,1 0))",
        1, 1, 0);
    test_one<point, MultiLineString, MultiLineString>("case_multi_ml_ml_2",
        "MULTILINESTRING((0 0,1 1),(0.5 0,1.5 1))", "MULTILINESTRING((0 1,1 0),(0.5 1,1.5 0))",
        4, 4, 0);

    test_one<point, LineString, MultiLineString>("case_multi_l_ml",
        "LINESTRING(0 0,1 1)", "MULTILINESTRING((0 1,1 0),(0.5 1,1.5 0))",
        2, 2, 0);
    test_one<point, MultiLineString, LineString>("case_multi_ml_l",
        "MULTILINESTRING((0 1,1 0),(0.5 1,1.5 0))", "LINESTRING(0 0,1 1)",
        2, 2, 0);

    test_one<LineString, MultiLineString, Box>("case_multi_ml_b",
        "MULTILINESTRING((0 0,3 3)(1 0,4 3))", "POLYGON((1 1,3 2))",
        2, 4, 2 * std::sqrt(2.0));
    test_one<LineString, Box, MultiLineString>("case_multi_b_ml",
        "POLYGON((1 1,3 2))", "MULTILINESTRING((0 0,3 3)(1 0,4 3))",
        2, 4, 2 * std::sqrt(2.0));
}

template <typename P>
void test_point_output()
{
    typedef bg::model::box<P> box;
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> multi_polygon;

    test_point_output<multi_polygon, multi_polygon>(case_multi_simplex[0], case_multi_simplex[1], 10);
    test_point_output<linestring, multi_polygon>("linestring(4 0,0 4)", case_multi_simplex[0], 4);
    test_point_output<box, multi_polygon>("box(3 0,4 6)", case_multi_simplex[0], 8);
}

template <typename MultiPolygon, typename MultiLineString>
void test_areal_linear()
{
    typedef typename boost::range_value<MultiPolygon>::type Polygon;
    typedef typename boost::range_value<MultiLineString>::type LineString;
    typedef typename bg::point_type<Polygon>::type Point;
    typedef bg::model::ring<Point> Ring;

    test_one_lp<LineString, MultiPolygon, LineString>("case_mp_ls_1", case_multi_simplex[0], "LINESTRING(2 0,2 5)", 2, 4, 3.70);
    test_one_lp<LineString, Polygon, MultiLineString>("case_p_mls_1", case_single_simplex, "MULTILINESTRING((2 0,2 5),(3 0,3 5))", 2, 4, 7.5);
    test_one_lp<LineString, MultiPolygon, MultiLineString>("case_mp_mls_1", case_multi_simplex[0], "MULTILINESTRING((2 0,2 5),(3 0,3 5))", 4, 8, 6.8333333);
    test_one_lp<LineString, Ring, MultiLineString>("case_r_mls_1", case_single_simplex, "MULTILINESTRING((2 0,2 5),(3 0,3 5))", 2, 4, 7.5);
}

template <typename P>
void test_all()
{
    typedef bg::model::box<P> box;
    typedef bg::model::ring<P> ring;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> multi_polygon;
    test_areal<ring, polygon, multi_polygon>();

#if ! defined(BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE)

    typedef bg::model::ring<P, false> ring_ccw;
    typedef bg::model::polygon<P, false> polygon_ccw;
    typedef bg::model::multi_polygon<polygon_ccw> multi_polygon_ccw;
    test_areal<ring_ccw, polygon_ccw, multi_polygon_ccw>();

    typedef bg::model::ring<P, true, false> ring_open;
    typedef bg::model::polygon<P, true, false> polygon_open;
    typedef bg::model::multi_polygon<polygon_open> multi_polygon_open;
    test_areal<ring_open, polygon_open, multi_polygon_open>();

    typedef bg::model::ring<P, false, false> ring_open_ccw;
    typedef bg::model::polygon<P, false, false> polygon_open_ccw;
    typedef bg::model::multi_polygon<polygon_open_ccw> multi_polygon_open_ccw;
    test_areal<ring_open_ccw, polygon_open_ccw, multi_polygon_open_ccw>();

    test_areal_clip<polygon, multi_polygon, box>();
    test_areal_clip<polygon_ccw, multi_polygon_ccw, box>();

    typedef bg::model::linestring<P> linestring;
    typedef bg::model::multi_linestring<linestring> multi_linestring;

    test_linear<linestring, multi_linestring, box>();
    test_areal_linear<multi_polygon, multi_linestring>();
#endif

    test_point_output<P>();
    // linear

}


int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();

#if ! defined(BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE)
    test_all<bg::model::d2::point_xy<float> >();

#if defined(HAVE_TTMATH)
    std::cout << "Testing TTMATH" << std::endl;
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

#endif

    return 0;
}
