// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


//#define BOOST_GEOMETRY_DEBUG_ENRICH
//#define BOOST_GEOMETRY_DEBUG_RELATIVE_ORDER

// Include the single-geometry version
#define BOOST_GEOMETRY_TEST_MULTI
#include <algorithms/overlay/traverse.cpp>



#include <boost/geometry/multi/core/closure.hpp>
#include <boost/geometry/multi/core/geometry_id.hpp>
#include <boost/geometry/multi/core/point_order.hpp>
#include <boost/geometry/multi/core/ring_type.hpp>

#include <boost/geometry/multi/algorithms/correct.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/multi/algorithms/detail/overlay/copy_segment_point.hpp>
#include <boost/geometry/multi/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/multi/algorithms/detail/sections/sectionalize.hpp>

#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/multi/views/detail/range_type.hpp>

#include <boost/geometry/multi/io/wkt/read.hpp>


#include "multi_overlay_cases.hpp"



template <typename MultiPolygon, bool Reverse>
void test_geometries()
{
    namespace ov = bg::detail::overlay;

    typedef test_traverse
        <
            MultiPolygon, MultiPolygon,
            ov::operation_intersection, Reverse, Reverse
        > test_traverse_intersection;
    typedef test_traverse
        <
            MultiPolygon, MultiPolygon,
            ov::operation_union, Reverse, Reverse
        > test_traverse_union;

    // Intersections:
    test_traverse_intersection::apply
        (
            "simplex", 2, 6.42,
            case_multi_simplex[0], case_multi_simplex[1]
        );

    test_traverse_intersection::apply
        (
            "case_65_multi", 1, 1,
            case_65_multi[0], case_65_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_66_multi", 1, 1,
            case_66_multi[0], case_66_multi[1]
        );

    test_traverse_intersection::apply
        (
            "case_67_multi", 1, 1,
            case_67_multi[0], case_67_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_69_multi", 1, 1,
            case_69_multi[0], case_69_multi[1]
        );

    test_traverse_intersection::apply
        (
            "case_71_multi", 2, 2,
            case_71_multi[0], case_71_multi[1]
        );

    // #72, note that it intersects into 2 shapes,
    // the third one is done by assemble (see intersection #72)
    test_traverse_intersection::apply
        (
            "case_72_multi", 2, 1.35,
            case_72_multi[0], case_72_multi[1]
        );

    test_traverse_intersection::apply
        (
            "case_73_multi", 2, 2,
            case_73_multi[0], case_73_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_74_multi", 2, 3,
            case_74_multi[0], case_74_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_75_multi", 1, 1,
            case_75_multi[0], case_75_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_77_multi", 5, 9,
            case_77_multi[0], case_77_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_78_multi", 2, 22, // Went from 3 to 2 by get_turns / partition
            case_78_multi[0], case_78_multi[1]
        );

    test_traverse_intersection::apply
        (
            "case_80_multi", 1, 0.5,
            case_80_multi[0], case_80_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_81_multi", 1, 0.25,
            case_81_multi[0], case_81_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_83_multi", 3, 1.25,
            case_83_multi[0], case_83_multi[1]
        );

    test_traverse_intersection::apply
        (
            "case_91_multi", 2, 1.0,
            case_91_multi[0], case_91_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_92_multi", 3, 1.5,
            case_92_multi[0], case_92_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_93_multi", 2, 1.25,
            case_93_multi[0], case_93_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_96_multi", 2, 1.0,
            case_96_multi[0], case_96_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_98_multi", 4, 3.0,
            case_98_multi[0], case_98_multi[1]
        );

    test_traverse_intersection::apply
        (
            "case_99_multi", 3, 1.75,
            case_99_multi[0], case_99_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_100_multi", 2, 1.5,
            case_100_multi[0], case_100_multi[1]
        );


    test_traverse_intersection::apply
        (
            "case_recursive_boxes_2", 1, 91,
            case_recursive_boxes_2[0], case_recursive_boxes_2[1]
        );
    test_traverse_intersection::apply
        (
            "case_107_multi", 2, 1.5,
            case_107_multi[0], case_107_multi[1]
        );
    test_traverse_intersection::apply
        (
            "case_recursive_boxes_3", 19, 12.5,
            case_recursive_boxes_3[0], case_recursive_boxes_3[1]
        );

    // Unions


    test_traverse_union::apply
        (
            "simplex", 1, 14.58,
            case_multi_simplex[0], case_multi_simplex[1]
        );

    test_traverse_union::apply
        (
            "case_61_multi", 1, 4,
            case_61_multi[0], case_61_multi[1]
        );
    test_traverse_union::apply
        (
            "case_62_multi", 1, 1 /*UU 2, 2 */,
            case_62_multi[0], case_62_multi[1]
        );
    test_traverse_union::apply
        (
            "case_63_multi", 1, 1 /*UU 2, 2 */,
            case_63_multi[0], case_63_multi[1]
        );
    test_traverse_union::apply
        (
            "case_64_multi", 1, 3,
            case_64_multi[0], case_64_multi[1]
        );

    test_traverse_union::apply
        (
            "case_66_multi", 1, 4 /*UU 3, 7 */,
            case_66_multi[0], case_66_multi[1]
        );
    test_traverse_union::apply
        (
            "case_68_multi", 1, 4 /*UU 2, 5 */,
            case_68_multi[0], case_68_multi[1]
        );
    // 71: single-polygon generates 2 shapes, multi-polygon
    //     generates 1 shape, both are self-tangent and OK
    test_traverse_union::apply
        (
            "case_71_multi", 1, 9,
            case_71_multi[0], case_71_multi[1]
        );

    test_traverse_union::apply
        (
            "case_72_multi", 1, 10.65,
            case_72_multi[0], case_72_multi[1]
        );

    test_traverse_union::apply
        (
            "case_73_multi", 1, 3,
            case_73_multi[0], case_73_multi[1]
        );
    test_traverse_union::apply
        (
            "case_74_multi", 2, 17,
            case_74_multi[0], case_74_multi[1]
        );
    test_traverse_union::apply
        (
            "case_75_multi", 1, 1 /*UU 5, 5 */,
            case_75_multi[0], case_75_multi[1]
        );
    test_traverse_union::apply
        (
            "case_76_multi", 2, 5 /*UU 6, 6 */,
            case_76_multi[0], case_76_multi[1]
        );

    test_traverse_union::apply
        (
            "case_80_multi", 1, 9.25,
            case_80_multi[0], case_80_multi[1]
        );
    test_traverse_union::apply
        (
            "case_81_multi", 1, 3.25,
            case_81_multi[0], case_81_multi[1]
        );
    test_traverse_union::apply
        (
            "case_82_multi", 3, 4,
            case_82_multi[0], case_82_multi[1]
        );
    test_traverse_union::apply
        (
            "case_84_multi", 1, 4,
            case_84_multi[0], case_84_multi[1]
        );
    test_traverse_union::apply
        (
            "case_85_multi", 1, 3.5,
            case_85_multi[0], case_85_multi[1]
        );
    test_traverse_union::apply
        (
            "case_86_multi", 1, 4,
            case_86_multi[0], case_86_multi[1]
        );
    test_traverse_union::apply
        (
            "case_87_multi", 1, 6,
            case_87_multi[0], case_87_multi[1]
        );
    test_traverse_union::apply
        (
            "case_88_multi", 2, 4,
            case_88_multi[0], case_88_multi[1]
        );
    test_traverse_union::apply
        (
            "case_89_multi", 1, 6,
            case_89_multi[0], case_89_multi[1]
        );
    test_traverse_union::apply
        (
            "case_90_multi", 1, 7.5,
            case_90_multi[0], case_90_multi[1]
        );
    test_traverse_union::apply
        (
            "case_92_multi", 2, 6.25,
            case_92_multi[0], case_92_multi[1]
        );
    test_traverse_union::apply
        (
            "case_94_multi", 1, 10.0,
            case_94_multi[0], case_94_multi[1]
        );
    test_traverse_union::apply
        (
            "case_95_multi", 2, 6.5,
            case_95_multi[0], case_95_multi[1]
        );
    test_traverse_union::apply
        (
            "case_96_multi", 1, 3.5,
            case_96_multi[0], case_96_multi[1]
        );
    test_traverse_union::apply
        (
            "case_97_multi", 1, 3.75,
            case_97_multi[0], case_97_multi[1]
        );
    test_traverse_union::apply
        (
            "case_101_multi", 1, 22.25,
            case_101_multi[0], case_101_multi[1]
        );
    test_traverse_union::apply
        (
            "case_102_multi", 3, 24.25,
            case_102_multi[0], case_102_multi[1]
        );
    test_traverse_union::apply
        (
            "case_103_multi", 1, 25,
            case_103_multi[0], case_103_multi[1]
        );
    test_traverse_union::apply
        (
            "case_104_multi", 1, 25,
            case_104_multi[0], case_104_multi[1]
        );
    test_traverse_union::apply
        (
            "case_105_multi", 1, 25,
            case_105_multi[0], case_105_multi[1]
        );
    test_traverse_union::apply
        (
            "case_106_multi", 1, 25,
            case_106_multi[0], case_106_multi[1]
        );


    test_traverse_union::apply
        (
            "case_recursive_boxes_1", 2, 97,
            case_recursive_boxes_1[0], case_recursive_boxes_1[1]
        );

    test_traverse_union::apply
        (
            "case_recursive_boxes_3", 7, 49.5,
            case_recursive_boxes_3[0], case_recursive_boxes_3[1]
        );

    test_traverse_intersection::apply
        (
            "pie_21_7_21_0_3", 2, 818824.56678,
            pie_21_7_21_0_3[0], pie_21_7_21_0_3[1]
        );

    test_traverse_intersection::apply
        (
            "pie_23_19_5_0_2", 2, 2948602.3911823,
            pie_23_19_5_0_2[0], pie_23_19_5_0_2[1]
        );
    test_traverse_intersection::apply
        (
            "pie_7_14_5_0_7", 2, 490804.56678,
            pie_7_14_5_0_7[0], pie_7_14_5_0_7[1]
        );
    test_traverse_intersection::apply
        (
            "pie_16_16_9_0_2", 2, 1146795,
            pie_16_16_9_0_2[0], pie_16_16_9_0_2[1]
        );
    test_traverse_intersection::apply
        (
            "pie_7_2_1_0_15", 2, 490585.5,
            pie_7_2_1_0_15[0], pie_7_2_1_0_15[1]
        );

}

template <typename T>
void test_all()
{
    typedef bg::model::point<T, 2, bg::cs::cartesian> point_type;

    typedef bg::model::multi_polygon
        <
            bg::model::polygon<point_type>
        > multi_polygon;

    typedef bg::model::multi_polygon
        <
            bg::model::polygon<point_type, false>
        > multi_polygon_ccw;

    test_geometries<multi_polygon, false>();
    test_geometries<multi_polygon_ccw, true>();
}


int test_main(int, char* [])
{
    test_all<double>();

    return 0;
}
