// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

//#define HAVE_TTMATH
//#define BOOST_GEOMETRY_DEBUG_ASSEMBLE
//#define BOOST_GEOMETRY_CHECK_WITH_SQLSERVER

//#define BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER
//#define BOOST_GEOMETRY_DEBUG_FOLLOW
//#define BOOST_GEOMETRY_DEBUG_TRAVERSE


#include <algorithms/test_difference.hpp>
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
            5, 21, 5.58, 4, 17, 2.58);

    test_one<Polygon, MultiPolygon, MultiPolygon>("case_multi_no_ip",
            case_multi_no_ip[0], case_multi_no_ip[1],
            2, 12, 24.0, 2, 12, 34.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_multi_2",
            case_multi_2[0], case_multi_2[1],
            2, 15, 19.6, 2, 13, 33.6);

    test_one<Polygon, MultiPolygon, Polygon>("simplex_multi_mp_p",
            case_multi_simplex[0], case_single_simplex,
            5, 21, 5.58, 4, 17, 2.58);
    test_one<Polygon, Ring, MultiPolygon>("simplex_multi_r_mp",
            case_single_simplex, case_multi_simplex[0],
            4, 17, 2.58, 5, 21, 5.58);
    test_one<Ring, MultiPolygon, Polygon>("simplex_multi_mp_r",
            case_multi_simplex[0], case_single_simplex,
            5, 21, 5.58, 4, 17, 2.58);

    // Constructed cases for multi/touch/equal/etc
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_61_multi",
            case_61_multi[0], case_61_multi[1],
            2, 10, 2, 2, 10, 2);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_62_multi",
            case_62_multi[0], case_62_multi[1],
            0, 0, 0, 1, 5, 1);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_63_multi",
            case_63_multi[0], case_63_multi[1],
            0, 0, 0, 1, 5, 1);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_64_multi",
        case_64_multi[0], case_64_multi[1],
            1, 5, 1, 1, 5, 1);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_65_multi",
        case_65_multi[0], case_65_multi[1],
            0, 0, 0, 2, 10, 3);
    /* TODO: fix
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_72_multi",
        case_72_multi[0], case_72_multi[1],
            3, 13, 1.65, 3, 17, 6.15);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_77_multi",
        case_77_multi[0], case_77_multi[1],
            6, 31, 7, 6, 36, 13);
    */

    test_one<Polygon, MultiPolygon, MultiPolygon>("case_78_multi",
        case_78_multi[0], case_78_multi[1],
            1, 5, 1.0, 1, 5, 1.0);

    // Ticket on GGL list 2011/10/25
    // to mix polygon/multipolygon in call to difference
    test_one<Polygon, Polygon, Polygon>("ggl_list_20111025_vd_pp",
        ggl_list_20111025_vd[0], ggl_list_20111025_vd[1],
            1, 4, 8.0, 1, 4, 12.5);
    test_one<Polygon, Polygon, MultiPolygon>("ggl_list_20111025_vd_pm",
        ggl_list_20111025_vd[0], ggl_list_20111025_vd[3],
            1, 4, 8.0, 1, 4, 12.5);
    test_one<Polygon, MultiPolygon, Polygon>("ggl_list_20111025_vd_mp",
        ggl_list_20111025_vd[2], ggl_list_20111025_vd[1],
            1, 4, 8.0, 1, 4, 12.5);
    test_one<Polygon, MultiPolygon, MultiPolygon>("ggl_list_20111025_vd_mm",
        ggl_list_20111025_vd[2], ggl_list_20111025_vd[3],
            1, 4, 8.0, 1, 4, 12.5);

    // Second case
    // This can be tested with this SQL for SQL-Server
    /*
    with viewy as (select geometry::STGeomFromText(
            'POLYGON((5 0,5 4,8 4,8 0,5 0))',0) as  p,
      geometry::STGeomFromText(
            'MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0)),((4 0,4 2,6 2,6 0,4 0)))',0) as q)
    select 
        p.STDifference(q).STArea(),p.STDifference(q).STNumGeometries(),p.STDifference(q) as p_min_q,
        q.STDifference(p).STArea(),q.STDifference(p).STNumGeometries(),q.STDifference(p) as q_min_p,
        p.STSymDifference(q).STArea(),q.STSymDifference(p) as p_xor_q
    from viewy

    Outputting:
    10, 1, <WKB>, 6, 2, <WKB>, 16, <WKB>
    */

    test_one<Polygon, Polygon, MultiPolygon>("ggl_list_20111025_vd_2",
        ggl_list_20111025_vd_2[0], ggl_list_20111025_vd_2[1],
            1, 7, 10.0, 2, 10, 6.0);

    test_one<Polygon, MultiPolygon, MultiPolygon>("ggl_list_20120915_h2_a",
        ggl_list_20120915_h2[0], ggl_list_20120915_h2[1],
            2, 13, 17.0, 0, 0, 0.0);
    test_one<Polygon, MultiPolygon, MultiPolygon>("ggl_list_20120915_h2_b",
        ggl_list_20120915_h2[0], ggl_list_20120915_h2[2],
            2, 13, 17.0, 0, 0, 0.0);

    test_one<Polygon, MultiPolygon, MultiPolygon>("ggl_list_20120221_volker",
        ggl_list_20120221_volker[0], ggl_list_20120221_volker[1],
            2, 12, 7962.66, 1, 18, 2775258.93, 
            0.001);


    /* TODO: fix
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_101_multi",
        case_101_multi[0], case_101_multi[1],
            5, 23, 4.75, 5, 40, 12.75);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_102_multi",
        case_102_multi[0], case_102_multi[1],
            2, 8, 0.75, 6, 25, 3.75);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_107_multi",
        case_107_multi[0], case_107_multi[1],
            2, 11, 2.25, 3, 14, 3.0);
    */
    /*
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_1",
        case_recursive_boxes_1[0], case_recursive_boxes_1[1],
            1, 1, 1, 1, 1, 1);
    test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_2",
        case_recursive_boxes_2[0], case_recursive_boxes_2[1],
            1, 1, 1, 1, 1, 1);

     test_one<Polygon, MultiPolygon, MultiPolygon>("case_recursive_boxes_3",
         case_recursive_boxes_3[0], case_recursive_boxes_3[1],
            1, 1, 1, 1, 1, 1);
*/
}

template <typename MultiPolygon, typename MultiLineString>
void test_areal_linear()
{
    typedef typename boost::range_value<MultiPolygon>::type Polygon;
    typedef typename boost::range_value<MultiLineString>::type LineString;
    typedef typename bg::point_type<Polygon>::type Point;
    typedef bg::model::ring<Point> Ring;

    test_one_lp<LineString, LineString, MultiPolygon>("case_mp_ls_1", "LINESTRING(2 0,2 5)", case_multi_simplex[0], 2, 4, 1.30);
    test_one_lp<LineString, MultiLineString, Polygon>("case_p_mls_1", "MULTILINESTRING((2 0,2 5),(3 0,3 5))", case_single_simplex, 3, 6, 2.5);
    test_one_lp<LineString, MultiLineString, MultiPolygon>("case_mp_mls_1", "MULTILINESTRING((2 0,2 5),(3 0,3 5))", case_multi_simplex[0], 5, 10, 3.1666667);
    test_one_lp<LineString, MultiLineString, Ring>("case_r_mls_1", "MULTILINESTRING((2 0,2 5),(3 0,3 5))", case_single_simplex, 3, 6, 2.5);

    // Collinear cases, with multiple turn points at the same location
    test_one_lp<LineString, LineString, MultiPolygon>("case_mp_ls_2a", "LINESTRING(1 0,1 1,2 1,2 0)", "MULTIPOLYGON(((0 0,0 1,1 1,1 0,0 0)),((1 1,1 2,2 2,2 1,1 1)))", 1, 2, 1.0);
    test_one_lp<LineString, LineString, MultiPolygon>("case_mp_ls_2b", "LINESTRING(1 0,1 1,2 1,2 0)", "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((0 0,0 1,1 1,1 0,0 0)))", 1, 2, 1.0);

    test_one_lp<LineString, LineString, MultiPolygon>("case_mp_ls_3", 
            "LINESTRING(6 6,6 7,7 7,7 6,8 6,8 7,9 7,9 6)", 
            "MULTIPOLYGON(((5 7,5 8,6 8,6 7,5 7)),((6 6,6 7,7 7,7 6,6 6)),((8 8,9 8,9 7,8 7,7 7,7 8,8 8)))", 2, 5, 3.0);

    return;

    // TODO: this case contains collinearities and should still be solved
    test_one_lp<LineString, LineString, MultiPolygon>("case_mp_ls_4", 
            "LINESTRING(0 5,0 6,1 6,1 5,2 5,2 6,3 6,3 5,3 4,3 3,2 3,2 4,1 4,1 3,0 3,0 4)", 
            "MULTIPOLYGON(((0 2,0 3,1 2,0 2)),((2 5,3 6,3 5,2 5)),((1 5,1 6,2 6,2 5,1 5)),((2 3,2 4,3 4,2 3)),((0 3,1 4,1 3,0 3)),((4 3,3 3,3 5,4 5,4 4,4 3)))", 5, 11, 6.0);
}


template <typename P>
void test_all()
{
    typedef bg::model::box<P> box;
    typedef bg::model::ring<P> ring;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::multi_polygon<polygon> multi_polygon;
    test_areal<ring, polygon, multi_polygon>();
    test_areal_linear<multi_polygon, bg::model::multi_linestring<bg::model::linestring<P> > >();
}


int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double > >();

#if ! defined(BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE)
    test_all<bg::model::d2::point_xy<float> >();

#if defined(HAVE_TTMATH)
    std::cout << "Testing TTMATH" << std::endl;
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

#endif

    return 0;
}
