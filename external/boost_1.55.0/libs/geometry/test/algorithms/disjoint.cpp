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

#include <iostream>
#include <string>


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include <test_common/test_point.hpp>

#include <algorithms/test_relate.hpp>


template <typename G1, typename G2>
void test_disjoint(std::string const& id,
            std::string const& wkt1,
            std::string const& wkt2, bool expected)
{
    G1 g1;
    bg::read_wkt(wkt1, g1);

    G2 g2;
    bg::read_wkt(wkt2, g2);

    bool detected = bg::disjoint(g1, g2);
    BOOST_CHECK_MESSAGE(detected == expected,
        "disjoint: " << id
        << " -> Expected: " << expected
        << " detected: " << detected);
}



template <typename P>
void test_all()
{
    typedef bg::model::box<P> box;

    test_disjoint<P, P>("pp1", "point(1 1)", "point(1 1)", false);
    test_disjoint<P, P>("pp2", "point(1 1)", "point(1.001 1)", true);

    // left-right
    test_disjoint<box, box>("bb1", "box(1 1, 2 2)", "box(3 1, 4 2)", true);
    test_disjoint<box, box>("bb2", "box(1 1, 2 2)", "box(2 1, 3 2)", false);
    test_disjoint<box, box>("bb3", "box(1 1, 2 2)", "box(2 2, 3 3)", false);
    test_disjoint<box, box>("bb4", "box(1 1, 2 2)", "box(2.001 2, 3 3)", true);

    // up-down
    test_disjoint<box, box>("bb5", "box(1 1, 2 2)", "box(1 3, 2 4)", true);
    test_disjoint<box, box>("bb6", "box(1 1, 2 2)", "box(1 2, 2 3)", false);
    // right-left
    test_disjoint<box, box>("bb7", "box(1 1, 2 2)", "box(0 1, 1 2)", false);
    test_disjoint<box, box>("bb8", "box(1 1, 2 2)", "box(0 1, 1 2)", false);

    // point-box
    test_disjoint<P, box>("pb1", "point(1 1)", "box(0 0, 2 2)", false);
    test_disjoint<P, box>("pb2", "point(2 2)", "box(0 0, 2 2)", false);
    test_disjoint<P, box>("pb3", "point(2.0001 2)", "box(1 1, 2 2)", true);
    test_disjoint<P, box>("pb4", "point(0.9999 2)", "box(1 1, 2 2)", true);

    // box-point (to test reverse compiling)
    test_disjoint<box, P>("bp1", "box(1 1, 2 2)", "point(2 2)", false);

    // Test triangles for polygons/rings, boxes
    // Note that intersections are tested elsewhere, they don't need
    // thorough test at this place
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::ring<P> ring;

    // Four times same test with other types
    test_disjoint<polygon, polygon>("disjoint_simplex_pp", disjoint_simplex[0], disjoint_simplex[1], true);
    test_disjoint<ring, polygon>("disjoint_simplex_rp", disjoint_simplex[0], disjoint_simplex[1], true);
    test_disjoint<ring, ring>("disjoint_simplex_rr", disjoint_simplex[0], disjoint_simplex[1], true);
    test_disjoint<polygon, ring>("disjoint_simplex_pr", disjoint_simplex[0], disjoint_simplex[1], true);

    // Testing touch
    test_disjoint<polygon, polygon>("touch_simplex_pp", touch_simplex[0], touch_simplex[1], false);

    // Testing overlap (and test compiling with box)
    test_disjoint<polygon, polygon>("overlaps_box_pp", overlaps_box[0], overlaps_box[1], false);
    test_disjoint<box, polygon>("overlaps_box_bp", overlaps_box[0], overlaps_box[1], false);
    test_disjoint<box, ring>("overlaps_box_br", overlaps_box[0], overlaps_box[1], false);
    test_disjoint<polygon, box>("overlaps_box_pb", overlaps_box[1], overlaps_box[0], false);
    test_disjoint<ring, box>("overlaps_box_rb", overlaps_box[1], overlaps_box[0], false);

    // Test if within(a,b) returns false for disjoint
    test_disjoint<ring, ring>("within_simplex_rr1", within_simplex[0], within_simplex[1], false);
    test_disjoint<ring, ring>("within_simplex_rr2", within_simplex[1], within_simplex[0], false);

    test_disjoint<P, ring>("point_ring1", "POINT(0 0)", "POLYGON((0 0,3 3,6 0,0 0))", false);
    test_disjoint<P, ring>("point_ring2", "POINT(3 1)", "POLYGON((0 0,3 3,6 0,0 0))", false);
    test_disjoint<P, ring>("point_ring3", "POINT(0 3)", "POLYGON((0 0,3 3,6 0,0 0))", true);
    test_disjoint<P, polygon>("point_polygon1", "POINT(0 0)", "POLYGON((0 0,3 3,6 0,0 0))", false);
    test_disjoint<P, polygon>("point_polygon2", "POINT(3 1)", "POLYGON((0 0,3 3,6 0,0 0))", false);
    test_disjoint<P, polygon>("point_polygon3", "POINT(0 3)", "POLYGON((0 0,3 3,6 0,0 0))", true);

    test_disjoint<ring, P>("point_ring2", "POLYGON((0 0,3 3,6 0,0 0))", "POINT(0 0)", false);
    test_disjoint<polygon, P>("point_polygon2", "POLYGON((0 0,3 3,6 0,0 0))", "POINT(0 0)", false);

    // Linear
    typedef bg::model::linestring<P> ls;
    typedef bg::model::segment<P> segment;
    test_disjoint<ls, ls>("ls/ls 1", "linestring(0 0,1 1)", "linestring(1 0,0 1)", false);
    test_disjoint<ls, ls>("ls/ls 2", "linestring(0 0,1 1)", "linestring(1 0,2 1)", true);
    test_disjoint<segment, segment>("s/s 1", "linestring(0 0,1 1)", "linestring(1 0,0 1)", false);
    test_disjoint<segment, segment>("s/s 2", "linestring(0 0,1 1)", "linestring(1 0,2 1)", true);

    // Test degenerate segments (patched by Karsten Ahnert on 2012-07-25)
    test_disjoint<segment, segment>("s/s 3", "linestring(0 0,0 0)", "linestring(1 0,0 1)", true);
    test_disjoint<segment, segment>("s/s 4", "linestring(0 0,0 0)", "linestring(0 0,0 0)", false);
    test_disjoint<segment, segment>("s/s 5", "linestring(0 0,0 0)", "linestring(1 0,1 0)", true);
    test_disjoint<segment, segment>("s/s 6", "linestring(0 0,0 0)", "linestring(0 1,0 1)", true);

    // Collinear opposite
    test_disjoint<ls, ls>("ls/ls co", "linestring(0 0,2 2)", "linestring(1 1,0 0)", false);
    // Collinear opposite and equal
    test_disjoint<ls, ls>("ls/ls co-e", "linestring(0 0,1 1)", "linestring(1 1,0 0)", false);


    // Problem described by Volker/Albert 2012-06-01
    test_disjoint<polygon, box>("volker_albert_1", 
        "POLYGON((1992 3240,1992 1440,3792 1800,3792 3240,1992 3240))", 
        "BOX(1941 2066, 2055 2166)", false);

    test_disjoint<polygon, box>("volker_albert_2", 
        "POLYGON((1941 2066,2055 2066,2055 2166,1941 2166))", 
        "BOX(1941 2066, 2055 2166)", false);

    // Degenerate linestrings
    {
        // Submitted by Zachary on the Boost.Geometry Mailing List, on 2012-01-29
        std::string const a = "linestring(100 10, 0 10)";
        std::string const b = "linestring(50 10, 50 10)"; // one point only, with same y-coordinate
        std::string const c = "linestring(100 10, 100 10)"; // idem, at left side
        test_disjoint<ls, ls>("dls/dls 1", a, b, false);
        test_disjoint<ls, ls>("dls/dls 2", b, a, false);
        test_disjoint<segment, segment>("ds/ds 1", a, b, false);
        test_disjoint<segment, segment>("ds/ds 2", b, a, false);
        test_disjoint<ls, ls>("dls/dls 1", a, c, false);
    }

    // Linestrings making angles normally ignored
    {
        // These (non-disjoint) cases 
        // correspond to the test "segment_intersection_collinear"

        // Collinear ('a')
        //       a1---------->a2
        // b1--->b2
        test_disjoint<ls, ls>("n1", "linestring(2 0,0 6)", "linestring(0 0,2 0)", false);

        //       a1---------->a2
        //                    b1--->b2
        test_disjoint<ls, ls>("n7", "linestring(2 0,6 0)", "linestring(6 0,8 0)", false);

        // Collinear - opposite ('f')
        //       a1---------->a2
        // b2<---b1
        test_disjoint<ls, ls>("o1", "linestring(2 0,6 0)", "linestring(2 0,0 0)", false);
    }

    {
        // Starting in the middle ('s')
        //           b2
        //           ^
        //           |
        //           |
        // a1--------b1----->a2
        test_disjoint<ls, ls>("case_s", "linestring(0 0,4 0)", "linestring(2 0,2 2)", false); 

        // Collinear, but disjoint
        test_disjoint<ls, ls>("c-d", "linestring(2 0,6 0)", "linestring(7 0,8 0)", true);

        // Parallel, disjoint
        test_disjoint<ls, ls>("c-d", "linestring(2 0,6 0)", "linestring(2 1,6 1)", true);

        // Error still there until 1.48 (reported "error", was reported to disjoint, so that's why it did no harm)
        test_disjoint<ls, ls>("case_recursive_boxes_1", 
            "linestring(10 7,10 6)", "linestring(10 10,10 9)", true);

    }

    // TODO test_disjoint<segment, ls>("s/ls 1", "linestring(0 0,1 1)", "linestring(1 0,0 1)", false);
    // TODO test_disjoint<segment, ls>("s/ls 2", "linestring(0 0,1 1)", "linestring(1 0,2 1)", true);
    // TODO test_disjoint<ls, segment>("ls/s 1", "linestring(0 0,1 1)", "linestring(1 0,0 1)", false);
    // TODO test_disjoint<ls, segment>("ls/s 2", "linestring(0 0,1 1)", "linestring(1 0,2 1)", true);

}


template <typename P>
void test_3d()
{
    typedef bg::model::box<P> box;

    test_disjoint<P, P>("pp 3d 1", "point(1 1 1)", "point(1 1 1)", false);
    test_disjoint<P, P>("pp 3d 2", "point(1 1 1)", "point(1.001 1 1)", true);

    test_disjoint<box, box>("bb1", "box(1 1 1, 2 2 2)", "box(3 1 1, 4 2 1)", true);
    test_disjoint<box, box>("bb2", "box(1 1 1, 2 2 2)", "box(2 1 1, 3 2 1)", false);
    test_disjoint<box, box>("bb3", "box(1 1 1, 2 2 2)", "box(2 2 1, 3 3 1)", false);
    test_disjoint<box, box>("bb4", "box(1 1 1, 2 2 2)", "box(2.001 2 1, 3 3 1)", true);

}

int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif

    test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();


    return 0;
}
