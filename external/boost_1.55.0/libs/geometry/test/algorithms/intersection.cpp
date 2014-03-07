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

#define TEST_ISOVIST

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

#include <boost/geometry/util/rational.hpp>

#include <algorithms/test_intersection.hpp>
#include <algorithms/test_overlay.hpp>

#include <algorithms/overlay/overlay_cases.hpp>

#include <test_common/test_point.hpp>
#include <test_common/with_pointer.hpp>
#include <test_geometries/custom_segment.hpp>

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)


static std::string pie_2_3_23_0[2] =
{
    "POLYGON((2500 2500,2855 3828,2500 3875,2500 2500))",
    "POLYGON((2500 2500,2791 3586,2499 3625,2208 3586,2500 2500))"
};

template <typename Polygon>
void test_areal()
{
    test_one<Polygon, Polygon, Polygon>("pie_2_3_23_0",
        pie_2_3_23_0[0], pie_2_3_23_0[1],
        1, 4, 163292.679042133, 0.1);

    test_one<Polygon, Polygon, Polygon>("simplex_with_empty_1",
        simplex_normal[0], polygon_empty,
        0, 0, 0.0);
    test_one<Polygon, Polygon, Polygon>("simplex_with_empty_2",
        polygon_empty, simplex_normal[0],
        0, 0, 0.0);

    test_one<Polygon, Polygon, Polygon>("simplex_normal",
        simplex_normal[0], simplex_normal[1],
        1, 7, 5.47363293);
    test_one<Polygon, Polygon, Polygon>("star_ring", example_star, example_ring,
        1, 18, 2.80983);

    test_one<Polygon, Polygon, Polygon>("star_poly", example_star, example_polygon,
        1, 0, // CLN: 23 points, other types: 22 point (one is merged)
        2.5020508);
    test_one<Polygon, Polygon, Polygon>("first_within_second1",
        first_within_second[0], first_within_second[1],
        1, 5, 1.0);

    test_one<Polygon, Polygon, Polygon>("first_within_second2",
        first_within_second[1], first_within_second[0],
        1, 5, 1.0);

    test_one<Polygon, Polygon, Polygon>("first_within_hole_of_second",
            first_within_hole_of_second[0], first_within_hole_of_second[1],
            0, 0, 0.0);

    // Two forming new hole
    test_one<Polygon, Polygon, Polygon>("new_hole",
        new_hole[0], new_hole[1],
        2, 10, 2.0);

    // Two identical
    test_one<Polygon, Polygon, Polygon>("identical",
        identical[0], identical[1],
        1, 5, 1.0);

    test_one<Polygon, Polygon, Polygon>("intersect_exterior_and_interiors_winded",
        intersect_exterior_and_interiors_winded[0], intersect_exterior_and_interiors_winded[1],
        1, 14, 25.2166667);

    test_one<Polygon, Polygon, Polygon>("intersect_holes_disjoint",
        intersect_holes_disjoint[0], intersect_holes_disjoint[1],
        1, 15, 18.0);

    test_one<Polygon, Polygon, Polygon>("intersect_holes_intersect",
        intersect_holes_intersect[0], intersect_holes_intersect[1],
        1, 14, 18.25);

    test_one<Polygon, Polygon, Polygon>("intersect_holes_intersect_and_disjoint",
        intersect_holes_intersect_and_disjoint[0], intersect_holes_intersect_and_disjoint[1],
        1, 19, 17.25);

    test_one<Polygon, Polygon, Polygon>("intersect_holes_intersect_and_touch",
        intersect_holes_intersect_and_touch[0], intersect_holes_intersect_and_touch[1],
        1, 23, 17.25);

    test_one<Polygon, Polygon, Polygon>("intersect_holes_new_ring",
        intersect_holes_new_ring[0], intersect_holes_new_ring[1],
        2, 23, 122.1039);

    test_one<Polygon, Polygon, Polygon>("winded",
        winded[0], winded[1],
        1, 22, 40.0);

    test_one<Polygon, Polygon, Polygon>("within_holes_disjoint",
        within_holes_disjoint[0], within_holes_disjoint[1],
        1, 15, 23.0);

    test_one<Polygon, Polygon, Polygon>("side_side",
        side_side[0], side_side[1],
        0, 0, 0.0);

    test_one<Polygon, Polygon, Polygon>("two_bends",
        two_bends[0], two_bends[1],
        1, 7, 24.0);

    test_one<Polygon, Polygon, Polygon>("star_comb_15",
        star_comb_15[0], star_comb_15[1],
        28, 150, 189.952883);

    test_one<Polygon, Polygon, Polygon>("simplex_normal",
        simplex_normal[0], simplex_normal[1],
        1, 7, 5.47363293);

    test_one<Polygon, Polygon, Polygon>("distance_zero",
        distance_zero[0], distance_zero[1],
        1, 0 /* f: 4, other: 5 */, 0.29516139, 0.01);

    test_one<Polygon, Polygon, Polygon>("equal_holes_disjoint",
        equal_holes_disjoint[0], equal_holes_disjoint[1],
        1, 20, 81 - 2 * 3 * 3 - 3 * 7);

    test_one<Polygon, Polygon, Polygon>("only_hole_intersections1",
        only_hole_intersections[0], only_hole_intersections[1],
        1, 21, 178.090909);
    test_one<Polygon, Polygon, Polygon>("only_hole_intersection2",
        only_hole_intersections[0], only_hole_intersections[2],
        1, 21, 149.090909);

    test_one<Polygon, Polygon, Polygon>("fitting",
        fitting[0], fitting[1],
        0, 0, 0);

    test_one<Polygon, Polygon, Polygon>("crossed",
        crossed[0], crossed[1],
        3, 0, 1.5);

    typedef typename bg::coordinate_type<Polygon>::type ct;
    bool const ccw = bg::point_order<Polygon>::value == bg::counterclockwise;
    bool const open = bg::closure<Polygon>::value == bg::open;


#ifdef TEST_ISOVIST
#ifdef _MSC_VER
    test_one<Polygon, Polygon, Polygon>("isovist",
        isovist1[0], isovist1[1],
        1, 20, 88.19203,
        if_typed_tt<ct>(0.01, 0.1));

    // SQL Server gives: 88.1920416352664
    // PostGIS gives:    88.19203677911

#endif
#endif

    //std::cout << typeid(ct).name() << std::endl;

    if (! ccw && open)
    {
        // Pointcount for ttmath/double (both 5) or float (4)
        // double returns 5 (since method append_no_dups_or_spikes)
        // but not for ccw/open. Those cases has to be adapted once, anyway, 
        // because for open always one point too much is generated...
        test_one<Polygon, Polygon, Polygon>("ggl_list_20110306_javier",
            ggl_list_20110306_javier[0], ggl_list_20110306_javier[1],
            1, if_typed<ct, float>(4, 5), 
            0.6649875, 
            if_typed<ct, float>(1.0, 0.01)); 
    }
        
    test_one<Polygon, Polygon, Polygon>("ggl_list_20110307_javier",
        ggl_list_20110307_javier[0], ggl_list_20110307_javier[1],
        1, 4, 0.4, 0.01);

    test_one<Polygon, Polygon, Polygon>("ggl_list_20110627_phillip",
        ggl_list_20110627_phillip[0], ggl_list_20110627_phillip[1],
        1, if_typed_tt<ct>(6, 5), 11151.6618);

#ifdef _MSC_VER // gcc/linux behaves differently
    if (! boost::is_same<ct, float>::value)
    {
        test_one<Polygon, Polygon, Polygon>("ggl_list_20110716_enrico",
            ggl_list_20110716_enrico[0], ggl_list_20110716_enrico[1],
            3, 
            if_typed<ct, float>(19, 21),
            35723.8506317139);
    }
#endif

    test_one<Polygon, Polygon, Polygon>("buffer_rt_f", buffer_rt_f[0], buffer_rt_f[1],
                1, 4,  0.00029437899183903937, 0.01);

    test_one<Polygon, Polygon, Polygon>("buffer_rt_g", buffer_rt_g[0], buffer_rt_g[1],
                1, 0, 2.914213562373);

    test_one<Polygon, Polygon, Polygon>("ticket_8254", ticket_8254[0], ticket_8254[1],
                1, 4, 3.63593e-08, 0.01);

    test_one<Polygon, Polygon, Polygon>("ticket_6958", ticket_6958[0], ticket_6958[1],
                1, 4, 4.34355e-05, 0.01);

    test_one<Polygon, Polygon, Polygon>("ticket_8652", ticket_8652[0], ticket_8652[1],
                1, 4, 0.0003, 0.00001);

    test_one<Polygon, Polygon, Polygon>("buffer_mp1", buffer_mp1[0], buffer_mp1[1],
                1, 31, 2.271707796);

    test_one<Polygon, Polygon, Polygon>("buffer_mp2", buffer_mp2[0], buffer_mp2[1],
                1, 29, 0.457126);

    return;


    test_one<Polygon, Polygon, Polygon>(
            "polygon_pseudo_line",
            "Polygon((0 0,0 4,4 4,4 0,0 0))",
            "Polygon((2 -2,2 -1,2 6,2 -2))",
            5, 22, 1.1901714);
}

template <typename Polygon, typename Box>
void test_areal_clip()
{
    test_one<Polygon, Box, Polygon>("boxring", example_box, example_ring,
        2, 12, 1.09125);
    test_one<Polygon, Polygon, Box>("boxring2", example_ring,example_box,
        2, 12, 1.09125);

    test_one<Polygon, Box, Polygon>("boxpoly", example_box, example_polygon,
        3, 19, 0.840166);

    test_one<Polygon, Box, Polygon>("poly1", example_box,
        "POLYGON((3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2))",
        2, 12, 1.09125);

    test_one<Polygon, Box, Polygon>("clip_poly2", example_box,
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 2.5,5.3 2.5,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))",
        2, 12, 1.00375);

    test_one<Polygon, Box, Polygon>("clip_poly3", example_box,
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 2.5,4.5 2.5,4.5 1.2,4.9 0.8,2.9 0.7,2 1.3))",
        2, 12, 1.00375);

    test_one<Polygon, Box, Polygon>("clip_poly4", example_box,
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 2.5,4.5 2.5,4.5 2.3,5.0 2.3,5.0 2.1,4.5 2.1,4.5 1.9,4.0 1.9,4.5 1.2,4.9 0.8,2.9 0.7,2 1.3))",
        2, 16, 0.860892);

    test_one<Polygon, Box, Polygon>("clip_poly5", example_box,
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 2.5,4.5 1.2,2.9 0.7,2 1.3))",
        2, 11, 0.7575961);

    test_one<Polygon, Box, Polygon>("clip_poly6", example_box,
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.0 3.0,5.0 2.0,2.9 0.7,2 1.3))",
        2, 13, 1.0744456);

    test_one<Polygon, Box, Polygon>("clip_poly7", "Box(0 0, 3 3)",
        "POLYGON((2 2, 1 4, 2 4, 3 3, 2 2))",
        1, 4, 0.75);
}


template <typename Box>
void test_boxes(std::string const& wkt1, std::string const& wkt2, double expected_area, bool expected_result)
{
    Box box1, box2;
    bg::read_wkt(wkt1, box1);
    bg::read_wkt(wkt2, box2);

    Box box_out;
    bool detected = bg::intersection(box1, box2, box_out);
    typename bg::default_area_result<Box>::type area = bg::area(box_out);

    BOOST_CHECK_EQUAL(detected, expected_result);
    if (detected && expected_result)
    {
        BOOST_CHECK_CLOSE(area, expected_area, 0.01);
    }
}


template <typename P>
void test_point_output()
{
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::box<P> box;
    typedef bg::model::segment<P> segment;

    test_point_output<polygon, polygon>(simplex_normal[0], simplex_normal[1], 6);
    test_point_output<box, polygon>("box(1 1,6 4)", simplex_normal[0], 4);
    test_point_output<linestring, polygon>("linestring(0 2,6 2)", simplex_normal[0], 2);
    // NYI because of sectionize:
    // test_point_output<segment, polygon>("linestring(0 2,6 2)", simplex_normal[0], 2);
    // NYI because needs special treatment:
    // test_point_output<box, box>("box(0 0,4 4)", "box(2 2,6 6)", 2);
}


template <typename Polygon, typename LineString>
void test_areal_linear()
{
    std::string const poly_simplex = "POLYGON((1 1,1 3,3 3,3 1,1 1))";

    test_one_lp<LineString, Polygon, LineString>("simplex", poly_simplex, "LINESTRING(0 2,4 2)", 1, 2, 2.0);
    test_one_lp<LineString, Polygon, LineString>("case2",   poly_simplex, "LINESTRING(0 1,4 3)", 1, 2, sqrt(5.0));
    test_one_lp<LineString, Polygon, LineString>("case3", "POLYGON((2 0,2 5,5 5,5 0,2 0))", "LINESTRING(0 1,1 2,3 2,4 3,6 3,7 4)", 1, 4, 2 + sqrt(2.0));
    test_one_lp<LineString, Polygon, LineString>("case4", "POLYGON((0 0,0 4,2 4,2 0,0 0))", "LINESTRING(1 1,3 2,1 3)", 2, 4, sqrt(5.0));

    test_one_lp<LineString, Polygon, LineString>("case5", poly_simplex, "LINESTRING(0 1,3 4)", 1, 2, sqrt(2.0));
    test_one_lp<LineString, Polygon, LineString>("case6", "POLYGON((2 0,2 4,3 4,3 1,4 1,4 3,5 3,5 1,6 1,6 3,7 3,7 1,8 1,8 3,9 3,9 0,2 0))", "LINESTRING(1 1,10 3)", 4, 8, 
            // Pieces are 1 x 2/9:
            4.0 * sqrt(1.0 + 4.0/81.0));
    test_one_lp<LineString, Polygon, LineString>("case7", poly_simplex, "LINESTRING(1.5 1.5,2.5 2.5)", 1, 2, sqrt(2.0));
    test_one_lp<LineString, Polygon, LineString>("case8", poly_simplex, "LINESTRING(1 0,2 0)", 0, 0, 0.0);

    std::string const poly_9 = "POLYGON((1 1,1 4,4 4,4 1,1 1))";
    test_one_lp<LineString, Polygon, LineString>("case9", poly_9, "LINESTRING(0 1,1 2,2 2)", 1, 2, 1.0);
    test_one_lp<LineString, Polygon, LineString>("case10", poly_9, "LINESTRING(0 1,1 2,0 2)", 0, 0, 0.0);
    test_one_lp<LineString, Polygon, LineString>("case11", poly_9, "LINESTRING(2 2,4 2,3 3)", 1, 3, 2.0 + sqrt(2.0));
    test_one_lp<LineString, Polygon, LineString>("case12", poly_9, "LINESTRING(2 3,4 4,5 6)", 1, 2, sqrt(5.0));

    test_one_lp<LineString, Polygon, LineString>("case13", poly_9, "LINESTRING(3 2,4 4,2 3)", 1, 3, 2.0 * sqrt(5.0));
    test_one_lp<LineString, Polygon, LineString>("case14", poly_9, "LINESTRING(5 6,4 4,6 5)", 0, 0, 0.0);
    test_one_lp<LineString, Polygon, LineString>("case15", poly_9, "LINESTRING(0 2,1 2,1 3,0 3)", 1, 2, 1.0);
    test_one_lp<LineString, Polygon, LineString>("case16", poly_9, "LINESTRING(2 2,1 2,1 3,2 3)", 1, 4, 3.0);

    std::string const angly = "LINESTRING(2 2,2 1,4 1,4 2,5 2,5 3,4 3,4 4,5 4,3 6,3 5,2 5,2 6,0 4)";
    test_one_lp<LineString, Polygon, LineString>("case17", "POLYGON((1 1,1 5,4 5,4 1,1 1))", angly, 3, 8, 6.0);
    test_one_lp<LineString, Polygon, LineString>("case18", "POLYGON((1 1,1 5,5 5,5 1,1 1))", angly, 2, 12, 10.0 + sqrt(2.0));
    test_one_lp<LineString, Polygon, LineString>("case19", poly_9, "LINESTRING(1 2,1 3,0 3)", 1, 2, 1.0);
    test_one_lp<LineString, Polygon, LineString>("case20", poly_9, "LINESTRING(1 2,1 3,2 3)", 1, 3, 2.0);

    test_one_lp<LineString, Polygon, LineString>("case21", poly_9, "LINESTRING(1 2,1 4,4 4,4 1,2 1,2 2)", 1, 6, 11.0);

    // Compile test - arguments in any order:
    test_one<LineString, Polygon, LineString>("simplex", poly_simplex, "LINESTRING(0 2,4 2)", 1, 2, 2.0);
    test_one<LineString, LineString, Polygon>("simplex", "LINESTRING(0 2,4 2)", poly_simplex, 1, 2, 2.0);

    typedef typename bg::point_type<Polygon>::type Point;
    test_one<LineString, bg::model::ring<Point>, LineString>("simplex", poly_simplex, "LINESTRING(0 2,4 2)", 1, 2, 2.0);

}


template <typename P>
void test_all()
{
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::box<P> box;
    typedef bg::model::segment<P> segment;

    typedef bg::model::polygon<P, false> polygon_ccw;
    typedef bg::model::polygon<P, true, false> polygon_open;
    typedef bg::model::polygon<P, false, false> polygon_ccw_open;

    std::string clip = "box(2 2,8 8)";

    test_areal_linear<polygon, linestring>();
    test_areal_linear<polygon_open, linestring>();
    test_areal_linear<polygon_ccw, linestring>();
    test_areal_linear<polygon_ccw_open, linestring>();

    // Test polygons clockwise and counter clockwise
    test_areal<polygon>();

    test_areal<polygon_ccw>();
    test_areal<polygon_open>();
    test_areal<polygon_ccw_open>();


    test_areal_clip<polygon, box>();
    test_areal_clip<polygon_ccw, box>();

#if defined(TEST_FAIL_DIFFERENT_ORIENTATIONS)
    // Should NOT compile
    // NOTE: this can probably be relaxed later on.
    test_one<polygon, polygon_ccw, polygon>("simplex_normal",
        simplex_normal[0], simplex_normal[1],
        1, 7, 5.47363293);
    // Output ccw, nyi (should be just reversing afterwards)
    test_one<polygon, polygon, polygon_ccw>("simplex_normal",
        simplex_normal[0], simplex_normal[1],
        1, 7, 5.47363293);
#endif

       // Basic check: box/linestring, is clipping OK? should compile in any order
    test_one<linestring, linestring, box>("llb", "LINESTRING(0 0,10 10)", clip, 1, 2, sqrt(2.0 * 6.0 * 6.0));
    test_one<linestring, box, linestring>("lbl", clip, "LINESTRING(0 0,10 10)", 1, 2, sqrt(2.0 * 6.0 * 6.0));

    // Box/segment
    test_one<linestring, segment, box>("lsb", "LINESTRING(0 0,10 10)", clip, 1, 2, sqrt(2.0 * 6.0 * 6.0));
    test_one<linestring, box, segment>("lbs", clip, "LINESTRING(0 0,10 10)", 1, 2, sqrt(2.0 * 6.0 * 6.0));

    // Completely inside
    test_one<linestring, linestring, box>("llbi", "LINESTRING(3 3,7 7)", clip, 1, 2, sqrt(2.0 * 4.0 * 4.0));

    // Completely outside
    test_one<linestring, linestring, box>("llbo", "LINESTRING(9 9,10 10)", clip, 0, 0, 0);

    // Touching with point (-> output linestring with ONE point)
    //std::cout << "Note: the output line is degenerate! Might be removed!" << std::endl;
    test_one<linestring, linestring, box>("llb_touch", "LINESTRING(8 8,10 10)", clip, 1, 1, 0.0);

    // Along border
    test_one<linestring, linestring, box>("llb_along", "LINESTRING(2 2,2 8)", clip, 1, 2, 6);

    // Outputting two lines (because of 3-4-5 constructions (0.3,0.4,0.5)
    // which occur 4 times, the length is expected to be 2.0)
    test_one<linestring, linestring, box>("llb_2", "LINESTRING(1.7 1.6,2.3 2.4,2.9 1.6,3.5 2.4,4.1 1.6)", clip, 2, 6, 4 * 0.5);

    // linear
    test_one<P, linestring, linestring>("llp1", "LINESTRING(0 0,1 1)", "LINESTRING(0 1,1 0)", 1, 1, 0);
    test_one<P, segment, segment>("ssp1", "LINESTRING(0 0,1 1)", "LINESTRING(0 1,1 0)", 1, 1, 0);
    test_one<P, linestring, linestring>("llp2", "LINESTRING(0 0,1 1)", "LINESTRING(0 0,2 2)", 2, 2, 0);

    // polygons outputing points
    //test_one<P, polygon, polygon>("ppp1", simplex_normal[0], simplex_normal[1], 1, 7, 5.47363293);

    test_boxes<box>("box(2 2,8 8)", "box(4 4,10 10)", 16, true);
    test_boxes<box>("box(2 2,8 7)", "box(4 4,10 10)", 12, true);
    test_boxes<box>("box(2 2,8 7)", "box(14 4,20 10)", 0, false);
    test_boxes<box>("box(2 2,4 4)", "box(4 4,8 8)", 0, true);

    test_point_output<P>();


    /*
    test_one<polygon, box, polygon>(99, "box(115041.10 471900.10, 118334.60 474523.40)",
            "POLYGON ((115483.40 474533.40, 116549.40 474059.20, 117199.90 473762.50, 117204.90 473659.50, 118339.40 472796.90, 118334.50 472757.90, 118315.10 472604.00, 118344.60 472520.90, 118277.90 472419.10, 118071.40 472536.80, 118071.40 472536.80, 117943.10 472287.70, 117744.90 472248.40, 117708.00 472034.50, 117481.90 472056.90, 117481.90 472056.90, 117272.30 471890.10, 117077.90 472161.20, 116146.60 473054.50, 115031.10 473603.30, 115483.40 474533.40))",
                1, 26, 3727690.74);
    */

}

void test_pointer_version()
{
    std::vector<test::test_point_xy*> ln;
    test::test_point_xy* p;
    p = new test::test_point_xy; p->x = 0; p->y = 0; ln.push_back(p);
    p = new test::test_point_xy; p->x = 10; p->y = 10; ln.push_back(p);

    bg::model::box<bg::model::d2::point_xy<double> > box;
    bg::assign_values(box, 2, 2, 8, 8);

    typedef bg::model::linestring<bg::model::d2::point_xy<double> > output_type;
    std::vector<output_type> clip;
    bg::detail::intersection::intersection_insert<output_type>(box, ln, std::back_inserter(clip));

    double length = 0;
    int n = 0;
    for (std::vector<output_type>::const_iterator it = clip.begin();
            it != clip.end(); ++it)
    {
        length += bg::length(*it);
        n += bg::num_points(*it);
    }

    BOOST_CHECK_EQUAL(clip.size(), 1u);
    BOOST_CHECK_EQUAL(n, 2);
    BOOST_CHECK_CLOSE(length, sqrt(2.0 * 6.0 * 6.0), 0.001);

    for (unsigned int i = 0; i < ln.size(); i++)
    {
        delete ln[i];
    }
}


template <typename P>
void test_exception()
{
    typedef bg::model::polygon<P> polygon;

    try
    {
        // Define polygon with a spike (= invalid)
        std::string spike = "POLYGON((0 0,0 4,2 4,2 6,2 4,4 4,4 0,0 0))";

        test_one<polygon, polygon, polygon>("with_spike",
            simplex_normal[0], spike,
            0, 0, 0);
    }
    catch(bg::overlay_invalid_input_exception const& )
    {
        return;
    }
    BOOST_CHECK_MESSAGE(false, "No exception thrown");
}

template <typename Point>
void test_rational()
{
    typedef bg::model::polygon<Point> polygon;
    test_one<polygon, polygon, polygon>("simplex_normal",
        simplex_normal[0], simplex_normal[1],
        1, 7, 5.47363293);
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

    test_exception<bg::model::d2::point_xy<double> >();
    test_pointer_version();
    test_rational<bg::model::d2::point_xy<boost::rational<int> > >();

    return 0;
}

