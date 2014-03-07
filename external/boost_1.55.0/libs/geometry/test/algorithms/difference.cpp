// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <iomanip>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/perimeter.hpp>

#include <boost/geometry/multi/algorithms/correct.hpp>
#include <boost/geometry/multi/algorithms/intersection.hpp>
#include <boost/geometry/multi/algorithms/within.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#include <boost/geometry/geometries/point_xy.hpp>

#include <algorithms/test_difference.hpp>
#include <algorithms/test_overlay.hpp>
#include <algorithms/overlay/overlay_cases.hpp>
#include <multi/algorithms/overlay/multi_overlay_cases.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>


#ifdef HAVE_TTMATH
#  include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif



template <typename Polygon, typename LineString>
void test_areal_linear()
{
    std::string const poly_simplex = "POLYGON((1 1,1 3,3 3,3 1,1 1))";
    test_one_lp<LineString, LineString, Polygon>("simplex", "LINESTRING(0 2,4 2)", poly_simplex, 2, 4, 2.0);
    test_one_lp<LineString, LineString, Polygon>("case2",  "LINESTRING(0 1,4 3)", poly_simplex, 2, 4, sqrt(5.0));
    test_one_lp<LineString, LineString, Polygon>("case3", "LINESTRING(0 1,1 2,3 2,4 3,6 3,7 4)", "POLYGON((2 0,2 5,5 5,5 0,2 0))", 2, 6, 2.0 + 2.0 * sqrt(2.0));
    test_one_lp<LineString, LineString, Polygon>("case4", "LINESTRING(1 1,3 2,1 3)", "POLYGON((0 0,0 4,2 4,2 0,0 0))", 1, 3, sqrt(5.0));

    test_one_lp<LineString, LineString, Polygon>("case5", "LINESTRING(0 1,3 4)", poly_simplex, 2, 4, 2.0 * sqrt(2.0));
    test_one_lp<LineString, LineString, Polygon>("case6", "LINESTRING(1 1,10 3)", "POLYGON((2 0,2 4,3 4,3 1,4 1,4 3,5 3,5 1,6 1,6 3,7 3,7 1,8 1,8 3,9 3,9 0,2 0))", 5, 10, 
            // Pieces are 1 x 2/9:
            5.0 * sqrt(1.0 + 4.0/81.0));


    test_one_lp<LineString, LineString, Polygon>("case7", "LINESTRING(1.5 1.5,2.5 2.5)", poly_simplex, 0, 0, 0.0);
    test_one_lp<LineString, LineString, Polygon>("case8", "LINESTRING(1 0,2 0)", poly_simplex, 1, 2, 1.0);

    std::string const poly_9 = "POLYGON((1 1,1 4,4 4,4 1,1 1))";
    test_one_lp<LineString, LineString, Polygon>("case9", "LINESTRING(0 1,1 2,2 2)", poly_9, 1, 2, sqrt(2.0));
    test_one_lp<LineString, LineString, Polygon>("case10", "LINESTRING(0 1,1 2,0 2)", poly_9, 1, 3, 1.0 + sqrt(2.0));
    test_one_lp<LineString, LineString, Polygon>("case11", "LINESTRING(2 2,4 2,3 3)", poly_9, 0, 0, 0.0);
    test_one_lp<LineString, LineString, Polygon>("case12", "LINESTRING(2 3,4 4,5 6)", poly_9, 1, 2, sqrt(5.0));

    test_one_lp<LineString, LineString, Polygon>("case13", "LINESTRING(3 2,4 4,2 3)", poly_9, 0, 0, 0.0);
    test_one_lp<LineString, LineString, Polygon>("case14", "LINESTRING(5 6,4 4,6 5)", poly_9, 1, 3, 2.0 * sqrt(5.0));

    test_one_lp<LineString, LineString, Polygon>("case15", "LINESTRING(0 2,1 2,1 3,0 3)", poly_9, 2, 4, 2.0);
    test_one_lp<LineString, LineString, Polygon>("case16", "LINESTRING(2 2,1 2,1 3,2 3)", poly_9, 0, 0, 0.0);

    std::string const angly = "LINESTRING(2 2,2 1,4 1,4 2,5 2,5 3,4 3,4 4,5 4,3 6,3 5,2 5,2 6,0 4)";
    test_one_lp<LineString, LineString, Polygon>("case17", angly, "POLYGON((1 1,1 5,4 5,4 1,1 1))", 3, 11, 6.0 + 4.0 * sqrt(2.0));
    test_one_lp<LineString, LineString, Polygon>("case18", angly, "POLYGON((1 1,1 5,5 5,5 1,1 1))", 2, 6, 2.0 + 3.0 * sqrt(2.0));
    test_one_lp<LineString, LineString, Polygon>("case19", "LINESTRING(1 2,1 3,0 3)", poly_9, 1, 2, 1.0);
    test_one_lp<LineString, LineString, Polygon>("case20", "LINESTRING(1 2,1 3,2 3)", poly_9, 0, 0, 0.0);

    test_one_lp<LineString, LineString, Polygon>("case21", "LINESTRING(1 2,1 4,4 4,4 1,2 1,2 2)", poly_9, 0, 0, 0.0);

    // More collinear (opposite) cases
    test_one_lp<LineString, LineString, Polygon>("case22", "LINESTRING(4 1,4 4,7 4)", poly_9, 1, 2, 3.0);
    test_one_lp<LineString, LineString, Polygon>("case23", "LINESTRING(4 0,4 4,7 4)", poly_9, 2, 4, 4.0);
    test_one_lp<LineString, LineString, Polygon>("case24", "LINESTRING(4 1,4 5,7 5)", poly_9, 1, 3, 4.0);
    test_one_lp<LineString, LineString, Polygon>("case25", "LINESTRING(4 0,4 5,7 5)", poly_9, 2, 5, 5.0);
    test_one_lp<LineString, LineString, Polygon>("case26", "LINESTRING(4 0,4 3,4 5,7 5)", poly_9, 2, 5, 5.0);
    test_one_lp<LineString, LineString, Polygon>("case27", "LINESTRING(4 4,4 5,5 5)", poly_9, 1, 3, 2.0);
}

template <typename P>
void test_all()
{
    typedef bg::model::box<P> box;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::ring<P> ring;

    typedef typename bg::coordinate_type<P>::type ct;

    test_areal_linear<polygon, linestring>();


    test_one<polygon, polygon, polygon>("simplex_normal",
        simplex_normal[0], simplex_normal[1],
        3, 12, 2.52636706856656,
        3, 12, 3.52636706856656);

    test_one<polygon, polygon, polygon>("simplex_with_empty",
        simplex_normal[0], polygon_empty,
        1, 4, 8.0,
        0, 0, 0.0);

    test_one<polygon, polygon, polygon>(
            "star_ring", example_star, example_ring,
            5, 22, 1.1901714,
            5, 27, 1.6701714);

    test_one<polygon, polygon, polygon>("two_bends",
        two_bends[0], two_bends[1],
        1, 5, 8.0,
        1, 5, 8.0);

    test_one<polygon, polygon, polygon>("star_comb_15",
        star_comb_15[0], star_comb_15[1],
        30, 160, 227.658275102812,
        30, 198, 480.485775259312);

    test_one<polygon, polygon, polygon>("new_hole",
        new_hole[0], new_hole[1],
        1, 9, 7.0,
        1, 13, 14.0);


    test_one<polygon, polygon, polygon>("crossed",
        crossed[0], crossed[1],
        1, 18, 19.5,
        1, 7, 2.5);

    test_one<polygon, polygon, polygon>("disjoint",
        disjoint[0], disjoint[1],
        1, 5, 1.0,
        1, 5, 1.0);

    test_one<polygon, polygon, polygon>("distance_zero",
        distance_zero[0], distance_zero[1],
        2, -1, 8.7048386,
        if_typed<ct, float>(1, 2), // The too small one is discarded for floating point
        -1, 0.0098387);


    test_one<polygon, polygon, polygon>("equal_holes_disjoint",
        equal_holes_disjoint[0], equal_holes_disjoint[1],
        1, 5, 9.0,
        1, 5, 9.0);

    test_one<polygon, polygon, polygon>("only_hole_intersections1",
        only_hole_intersections[0], only_hole_intersections[1],
        2, 10,  1.9090909,
        4, 16, 10.9090909);

    test_one<polygon, polygon, polygon>("only_hole_intersection2",
        only_hole_intersections[0], only_hole_intersections[2],
        3, 20, 30.9090909,
        4, 16, 10.9090909);

    test_one<polygon, polygon, polygon>("first_within_second",
        first_within_second[1], first_within_second[0],
        1, 10, 24,
        0, 0, 0);

    test_one<polygon, polygon, polygon>("fitting",
        fitting[0], fitting[1],
        1, 9, 21.0,
        1, 4, 4.0);

    test_one<polygon, polygon, polygon>("identical",
        identical[0], identical[1],
        0, 0, 0.0,
        0, 0, 0.0);

    test_one<polygon, polygon, polygon>("intersect_exterior_and_interiors_winded",
        intersect_exterior_and_interiors_winded[0], intersect_exterior_and_interiors_winded[1],
        4, 20, 11.533333,
        5, 26, 29.783333);

    test_one<polygon, polygon, polygon>("intersect_holes_intersect_and_disjoint",
        intersect_holes_intersect_and_disjoint[0], intersect_holes_intersect_and_disjoint[1],
        2, 16, 15.75,
        3, 17, 6.75);

    test_one<polygon, polygon, polygon>("intersect_holes_intersect_and_touch",
        intersect_holes_intersect_and_touch[0], intersect_holes_intersect_and_touch[1],
        3, 21, 16.25,
        3, 17, 6.25);

    test_one<polygon, polygon, polygon>("intersect_holes_new_ring",
        intersect_holes_new_ring[0], intersect_holes_new_ring[1],
        3, 15, 9.8961,
        4, 25, 121.8961, 0.01);

    test_one<polygon, polygon, polygon>("first_within_hole_of_second",
        first_within_hole_of_second[0], first_within_hole_of_second[1],
        1, 5, 1,
        1, 10, 16);

    test_one<polygon, polygon, polygon>("intersect_holes_disjoint",
        intersect_holes_disjoint[0], intersect_holes_disjoint[1],
        2, 14, 16.0,
        2, 10, 6.0);

    test_one<polygon, polygon, polygon>("intersect_holes_intersect",
        intersect_holes_intersect[0], intersect_holes_intersect[1],
        2, 16, 15.75,
        2, 12, 5.75);

    test_one<polygon, polygon, polygon>(
            "case4", case_4[0], case_4[1],
            6, 28, 2.77878787878788,
            4, 22, 4.77878787878788);

    test_one<polygon, polygon, polygon>(
            "case5", case_5[0], case_5[1],
            8, 36, 2.43452380952381,
            7, 33, 3.18452380952381);

    test_one<polygon, polygon, polygon>("winded",
        winded[0], winded[1],
        3, 37, 61,
        1, 15, 13);

    test_one<polygon, polygon, polygon>("within_holes_disjoint",
        within_holes_disjoint[0], within_holes_disjoint[1],
        2, 15, 25,
        1, 5, 1);

    test_one<polygon, polygon, polygon>("side_side",
        side_side[0], side_side[1],
        1, 5, 1,
        1, 5, 1);

    test_one<polygon, polygon, polygon>("buffer_mp1", 
        buffer_mp1[0], buffer_mp1[1],
        1, 61, 10.2717,
        1, 61, 10.2717);

    if (boost::is_same<ct, double>::value)
    {
        test_one<polygon, polygon, polygon>("buffer_mp2", 
            buffer_mp2[0], buffer_mp2[1],
            1, 91, 12.09857,
            1, 156, 24.19787);
    }

    /*** TODO: self-tangencies for difference
    test_one<polygon, polygon, polygon>("wrapped_a",
        wrapped[0], wrapped[1],
        3, 1, 61,
        1, 0, 13);

    test_one<polygon, polygon, polygon>("wrapped_b",
        wrapped[0], wrapped[2],
        3, 1, 61,
        1, 0, 13);
    ***/

#ifdef _MSC_VER
#ifdef TEST_ISOVIST
    test_one<polygon, polygon, polygon>("isovist",
        isovist1[0], isovist1[1],
        if_typed_tt<ct>(4, 2), 0, 0.279121891701124,
        if_typed_tt<ct>(4, 3), 0, if_typed_tt<ct>(224.889211358929, 223.777),
        if_typed_tt<ct>(0.001, 0.2));

    // SQL Server gives: 0.279121891701124 and 224.889211358929
    // PostGIS gives:    0.279121991127244 and 224.889205853156

#endif

    test_one<polygon, polygon, polygon>("ggl_list_20110306_javier",
        ggl_list_20110306_javier[0], ggl_list_20110306_javier[1],
        1, -1, 71495.3331,
        2, -1, 8960.49049); 
#endif
        
    test_one<polygon, polygon, polygon>("ggl_list_20110307_javier",
        ggl_list_20110307_javier[0], ggl_list_20110307_javier[1],
        1, if_typed<ct, float>(14, 13), 16815.6,
        1, 4, 3200.4,
        0.01);

    if (! boost::is_same<ct, float>::value)
    {
        test_one<polygon, polygon, polygon>("ggl_list_20110716_enrico",
            ggl_list_20110716_enrico[0], ggl_list_20110716_enrico[1],
            3, -1, 35723.8506317139,
            1, -1, 58456.4964294434
            );
    }

    test_one<polygon, polygon, polygon>("ggl_list_20110820_christophe",
        ggl_list_20110820_christophe[0], ggl_list_20110820_christophe[1],
        1, -1, 2.8570121719168924,
        1, -1, 64.498061986388564); 

    test_one<polygon, polygon, polygon>("ggl_list_20120717_volker",
        ggl_list_20120717_volker[0], ggl_list_20120717_volker[1],
        1, 11, 3370866.2295081965,
        1, 5, 384.2295081964694, 0.01); 
             
#ifdef _MSC_VER
    // 2011-07-02
    // Interesting FP-precision case.
    // sql server gives: 6.62295817619452E-05
    // PostGIS gives: 0.0 (no output)
    // Boost.Geometry gives results depending on FP-type, and compiler, and operating system.
    // For double, it is zero (skipped). On gcc/Linux, for float either.
    // Because we cannot predict this, we only test for MSVC
    if (boost::is_same<ct, double>::value
#if defined(HAVE_TTMATH)
        || boost::is_same<ct, ttmath_big>::value
#endif
        )
    {
        test_one<polygon, polygon, polygon>("ggl_list_20110627_phillip",
            ggl_list_20110627_phillip[0], ggl_list_20110627_phillip[1],
                if_typed_tt<ct>(1, 0), -1, 
                if_typed_tt<ct>(0.0000000000001105367, 0.0), 
            1, -1, 3577.40960816756,
            0.01
            );
    }
#endif

    // Other combi's
    {
        test_one<polygon, polygon, ring>(
                "star_ring_ring", example_star, example_ring,
                5, 22, 1.1901714, 5, 27, 1.6701714);

        test_one<polygon, ring, polygon>(
                "ring_star_ring", example_ring, example_star,
                5, 27, 1.6701714, 5, 22, 1.1901714);

        static std::string const clip = "POLYGON((2.5 0.5,5.5 2.5))";

        test_one<polygon, box, ring>("star_box",
            clip, example_star,
            4, 20, 2.833333, 4, 16, 0.833333);

        test_one<polygon, ring, box>("box_star",
            example_star, clip,
            4, 16, 0.833333, 4, 20, 2.833333);
    }

    // Counter clockwise
    {
        typedef bg::model::polygon<P, false> polygon_ccw;
        test_one<polygon, polygon_ccw, polygon_ccw>(
                "star_ring_ccw", example_star, example_ring,
                5, 22, 1.1901714, 5, 27, 1.6701714);
        test_one<polygon, polygon, polygon_ccw>(
                "star_ring_ccw1", example_star, example_ring,
                5, 22, 1.1901714, 5, 27, 1.6701714);
        test_one<polygon, polygon_ccw, polygon>(
                "star_ring_ccw2", example_star, example_ring,
                5, 22, 1.1901714, 5, 27, 1.6701714);
    }



    // Multi/box (should be moved to multi)
    {
        /* Tested with SQL Geometry:
                with viewy as (select geometry::STGeomFromText(
                        'MULTIPOLYGON(((0 1,2 5,5 3,0 1)),((1 1,5 2,5 0,1 1)))',0) as  p,
                  geometry::STGeomFromText(
                        'POLYGON((2 2,2 4,4 4,4 2,2 2))',0) as q)
                  
                select 
                    p.STDifference(q).STArea(),p.STDifference(q).STNumGeometries(),p.STDifference(q) as p_min_q,
                    q.STDifference(p).STArea(),q.STDifference(p).STNumGeometries(),q.STDifference(p) as q_min_p,
                    p.STSymDifference(q).STArea(),q.STSymDifference(p) as p_xor_q
                from viewy

        */
        typedef bg::model::multi_polygon<polygon> mp;

        static std::string const clip = "POLYGON((2 2,4 4))";

        test_one<polygon, box, mp>("simplex_multi_box_mp",
            clip, case_multi_simplex[0],
            2, -1, 0.53333333333, 3, -1, 8.53333333333);
        test_one<polygon, mp, box>("simplex_multi_mp_box",
            case_multi_simplex[0], clip,
            3, -1, 8.53333333333, 2, -1, 0.53333333333);

    }

    /***
    Experimental (cut), does not work:
    test_one<polygon, polygon, polygon>(
            "polygon_pseudo_line",
            "POLYGON((0 0,0 4,4 4,4 0,0 0))",
            "POLYGON((2 -2,2 -1,2 6,2 -2))",
            5, 22, 1.1901714,
            5, 27, 1.6701714);
    ***/
}

/*******
// To be moved to another file
template <typename T>
void test_difference_parcel_precision()
{
    typedef bg::model::d2::point_xy<T> point_type;
    typedef bg::model::polygon<point_type> polygon_type;
    typedef bg::model::linestring<point_type> linestring_type;
    typedef std::vector<boost::uint8_t> byte_vector;

    polygon_type parcel, buffer;

    {
        byte_vector wkb;
        bg::hex2wkb("0103000000010000001A00000023DBF97EE316064146B6F3FDD32513415A643BDFD216064175931804E225134196438B6C52150641C976BE9F34261341F6285C8F06150641022B871641261341F853E3A5D3140641E17A14AE50261341B81E85EBFB120641A4703D8A142713414E6210584D120641AC1C5A64602713414E621058431106414E621058C9271341A01A2FDD1B11064121B07268D8271341F4FDD478571006411904560ECF271341448B6CE7DD0F06418195438BC1271341F6285C8F6A0F06413BDF4F0DA72713410E2DB29D360F06416F1283C091271341E5D022DB070F0641F6285C0F7227134160E5D022DA0E06416F1283404F271341448B6CE7D30E0641F853E3A52427134154E3A59BE60E06411904568E07271341643BDF4F0C0F0641D7A3703DFC2613414A0C022B100F064125068115FB26134191ED7C3F310F0641B6F3FDD4F42613414C378941F11006414A0C022BA0261341EC51B81ECC1206413BDF4F0D40261341022B87167514064125068115F1251341B4C876BE8C160641C74B37897F25134121B07268C6160641508D976E7525134123DBF97EE316064146B6F3FDD3251341", std::back_inserter(wkb));
        bg::read_wkb(wkb.begin(), wkb.end(), parcel);
    }
    {
        byte_vector wkb;
        bg::hex2wkb("01030000000100000083000000000032FCD716064100009E998F25134100000706D81606410040A4998F2513410000DA0FD816064100C0E6998F2513410000A819D81606410080659A8F25134100806A23D816064100C0209B8F25134100801E2DD81606410080189C8F2513410000BE36D816064100404D9D8F25134100004540D816064100C0BE9E8F2513410000AF49D816064100806DA08F2513410000F752D8160641008059A28F2513410000195CD816064100C082A48F25134100800F65D81606410080E9A68F2513410000D66DD816064100408EA98F25134100006876D816064100C070AC8F2513410000C17ED8160641000091AF8F2513410080DC86D816064100C0EFB28F25134100009E8ED816064100C081B68F2513410080EC95D816064100803ABA8F2513410080C79CD8160641000018BE8F25134100002FA3D8160641008017C28F251341008022A9D8160641000037C68F2513410080A1AED8160641000074CA8F2513410000ACB3D81606410040CCCE8F251341000042B8D816064100403DD38F251341000062BCD81606410000C5D78F25134100000DC0D8160641000061DC8F251341000042C3D816064100000FE18F251341000001C6D81606410080CCE58F251341008049C8D8160641004097EA8F25134100001BCAD816064100006DEF8F251341008075CBD816064100804BF48F251341008058CCD8160641004030F98F2513410000C4CCD8160641000019FE8F2513410080B7CCD81606410080030390251341008032CCD81606410000ED0790251341000035CBD81606410000D40C902513410080BEC9D81606410040B511902513410000CFC7D816064100408F1690251341008065C5D816064100005F1B90251341008082C2D81606410080222090251341000025BFD81606410080D7249025134100004DBBD816064100807B29902513410080FAB6D816064100800C2E9025134100002DB2D816064100C08732902513410080E3ACD81606410000EB369025134100801EA7D81606410000343B902513410000DEA0D81606410080603F902513410080209AD816064100406E43902513410080209AC015064100406E43302613410080FC92C015064100004F473026134100008B8BC01506410040F64A302613410000D083C015064100C0634E302613410000D17BC0150641008097513026134100009273C015064100409154302613410000186BC015064100C050573026134100806762C01506410000D6593026134100808559C01506410000215C3026134100007650C01506410000315E3026134100003E47C015064100800660302613410000E23DC01506410000A1613026134100006734C015064100800063302613410080D12AC015064100C024643026134100002621C015064100800D653026134100006917C015064100C0BA653026134100809F0DC015064100402C66302613410000CE03C015064100006266302613410000F9F9BF15064100C05B6630261341000026F0BF1506410040196630261341000058E6BF15064100809A6530261341008095DCBF1506410040DF64302613410080E1D2BF1506410080E76330261341000042C9BF15064100C0B262302613410000BBBFBF1506410040416130261341000051B6BF1506410080925F30261341000009ADBF1506410080A65D302613410000E7A3BF15064100407D5B302613410080F09ABF150641008016593026134100002A92BF15064100C071563026134100009889BF15064100408F533026134100003F81BF15064100006F503026134100802379BF1506410040104D3026134100006271BF15064100407E49302613410080136ABF1506410080C5453026134100803863BF1506410000E841302613410000D15CBF1506410080E83D302613410080DD56BF1506410000C9393026134100805E51BF15064100008C35302613410000544CBF15064100C03331302613410000BE47BF15064100C0C22C3026134100009E43BF15064100003B28302613410000F33FBF15064100009F23302613410000BE3CBF1506410000F11E302613410000FF39BF1506410080331A302613410080B637BF15064100C06815302613410000E535BF150641000093103026134100808A34BF1506410080B40B302613410080A733BF15064100C0CF063026134100003C33BF1506410000E7013026134100804833BF1506410080FCFC2F2613410080CD33BF150641000013F82F2613410000CB34BF15064100002CF32F26134100804136BF15064100C04AEE2F26134100003138BF15064100C070E92F26134100809A3ABF1506410000A1E42F26134100807D3DBF1506410080DDDF2F2613410000DB40BF150641008028DB2F2613410000B344BF150641008084D62F26134100800549BF1506410080F3D12F2613410000D34DBF150641004078CD2F26134100801C53BF150641000015C92F2613410080E158BF1506410000CCC42F2613410000225FBF15064100809FC02F2613410080DF65BF15064100C091BC2F2613410080DF65D716064100C091BC8F2513410080036DD71606410000B1B88F25134100007574D716064100C009B58F2513410000307CD716064100409CB18F25134100002F84D7160641008068AE8F25134100006E8CD716064100C06EAB8F2513410000E894D71606410040AFA88F2513410080989DD716064100002AA68F25134100807AA6D71606410000DFA38F25134100008AAFD71606410000CFA18F2513410000C2B8D71606410080F99F8F25134100001EC2D716064100005F9E8F251341000099CBD71606410080FF9C8F25134100802ED5D71606410040DB9B8F2513410000DADED71606410080F29A8F251341000097E8D71606410040459A8F251341008060F2D716064100C0D3998F251341000032FCD716064100009E998F251341", std::back_inserter(wkb));
        bg::read_wkb(wkb.begin(), wkb.end(), buffer);
    }
    bg::correct(parcel);
    bg::correct(buffer);

    std::vector<polygon_type> pieces;
    bg::difference(parcel, buffer, pieces);

    std::vector<polygon_type> filled_out;
    bg::difference(parcel, pieces.back(), filled_out);

#if defined(TEST_OUTPUT)
    std::cout << bg::area(parcel) << std::endl;
    std::cout << bg::area(buffer) << std::endl;
    std::cout << pieces.size() << std::endl;
    std::cout << bg::area(pieces.front()) << std::endl;
    std::cout << filled_out.size() << std::endl;
    std::cout << std::setprecision(16) << bg::wkt(filled_out.front()) << std::endl;
    std::cout << bg::wkt(filled_out.front()) << std::endl;
    std::cout << bg::area(filled_out.front()) << std::endl;
    std::cout << bg::perimeter(filled_out.front()) << std::endl;
#endif

#if defined(TEST_WITH_SVG)
    {
        linestring_type cut_line;
        bg::read_wkt("linestring(180955 313700,180920 313740)", cut_line);

        std::ostringstream filename;
        filename << "difference_precision_"
            << string_from_type<T>::name()
            << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<point_type> mapper(svg, 500, 500);

        mapper.add(cut_line);

        //mapper.map(parcel, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:3");
        mapper.map(pieces.front(), "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:1");
        mapper.map(pieces.back(), "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:1");
        mapper.map(filled_out.front(), "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:3");

        mapper.map(cut_line, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:5;stroke-dasharray:1,7;stroke-linecap:round");
        //mapper.map(cut_line, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:2");
    }
#endif
}
*****/


template <typename P, bool clockwise, bool closed>
void test_specific()
{
    typedef bg::model::polygon<P, clockwise, closed> polygon;

    test_one<polygon, polygon, polygon>("ggl_list_20120717_volker",
        ggl_list_20120717_volker[0], ggl_list_20120717_volker[1],
        1, 11, 3370866.2295081965,
        1, 5, 384, 0.01); 
}


int test_main(int, char* [])
{
    //test_difference_parcel_precision<float>();
    //test_difference_parcel_precision<double>();

    test_all<bg::model::d2::point_xy<double> >();

    test_specific<bg::model::d2::point_xy<int>, false, false>();

#if ! defined(BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE)
    test_all<bg::model::d2::point_xy<float> >();

#ifdef HAVE_TTMATH
    std::cout << "Testing TTMATH" << std::endl;
    test_all<bg::model::d2::point_xy<ttmath_big> >();
    //test_difference_parcel_precision<ttmath_big>();
#endif
#endif

    return 0;
}
