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


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/assign.hpp>

#include <boost/geometry/strategies/cartesian/cart_intersect.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>

#include <boost/geometry/policies/relate/intersection_points.hpp>
#include <boost/geometry/policies/relate/direction.hpp>
#include <boost/geometry/policies/relate/de9im.hpp>
#include <boost/geometry/policies/relate/tupled.hpp>

#include <boost/geometry/algorithms/intersection.hpp>


#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>

template <typename IntersectionPoints>
static int check(IntersectionPoints const& is,
                std::size_t index, double expected_x, double expected_y)
{
    if (expected_x != -99 && expected_y != -99 && is.count > index)
    {
        double x = bg::get<0>(is.intersections[index]);
        double y = bg::get<1>(is.intersections[index]);

        BOOST_CHECK_CLOSE(x, expected_x, 0.001);
        BOOST_CHECK_CLOSE(y, expected_y, 0.001);
        return 1;
    }
    return 0;
}


template <typename P>
static void test_segment_intersection(std::string const& case_id,
                int x1, int y1, int x2, int y2,
                int x3, int y3, int x4, int y4,
                char expected_how, bool expected_opposite,
                int expected_arrival1, int expected_arrival2,
                int expected_x1, int expected_y1,
                int expected_x2 = -99, int expected_y2 = -99)

{
    typedef typename bg::coordinate_type<P>::type coordinate_type;
    typedef bg::model::referring_segment<const P> segment_type;

    P p1, p2, p3, p4;
    bg::assign_values(p1, x1, y1);
    bg::assign_values(p2, x2, y2);
    bg::assign_values(p3, x3, y3);
    bg::assign_values(p4, x4, y4);

    segment_type s12(p1,p2);
    segment_type s34(p3,p4);

    // Get the intersection point (or two points)
    bg::segment_intersection_points<P> is
        = bg::strategy::intersection::relate_cartesian_segments
        <
            bg::policies::relate::segments_intersection_points
                <
                    segment_type,
                    segment_type,
                    bg::segment_intersection_points<P>
                >
        >::apply(s12, s34);

    // Get just a character for Left/Right/intersects/etc, purpose is more for debugging
    bg::policies::relate::direction_type dir
        = bg::strategy::intersection::relate_cartesian_segments
        <
            bg::policies::relate::segments_direction
                <
                    segment_type,
                    segment_type
                >
        >::apply(s12, s34);

    int expected_count =
        check(is, 0, expected_x1, expected_y1)
        + check(is, 1, expected_x2, expected_y2);

    BOOST_CHECK_EQUAL(is.count, expected_count);
    BOOST_CHECK_EQUAL(dir.how, expected_how);
    BOOST_CHECK_EQUAL(dir.opposite, expected_opposite);
    BOOST_CHECK_EQUAL(dir.arrival[0], expected_arrival1);
    BOOST_CHECK_EQUAL(dir.arrival[1], expected_arrival2);
}


template <typename P>
void test_all()
{
    // Collinear - non opposite

    //       a1---------->a2
    // b1--->b2
    test_segment_intersection<P>("n1",
        2, 0, 6, 0,
        0, 0, 2, 0,
        'a', false, -1, 0,
        2, 0);

    //       a1---------->a2
    //    b1--->b2
    test_segment_intersection<P>("n2",
        2, 0, 6, 0,
        1, 0, 3, 0,
        'c', false, -1, 1,
        2, 0, 3, 0);

    //       a1---------->a2
    //       b1--->b2
    test_segment_intersection<P>("n3",
        2, 0, 6, 0,
        2, 0, 4, 0,
        'c', false, -1, 1,
        2, 0, 4, 0);

    //       a1---------->a2
    //          b1--->b2
    test_segment_intersection<P>("n4",
        2, 0, 6, 0,
        3, 0, 5, 0,
        'c', false, -1, 1,
        3, 0, 5, 0);

    //       a1---------->a2
    //              b1--->b2
    test_segment_intersection<P>("n5",
        2, 0, 6, 0,
        4, 0, 6, 0,
        'c', false, 0, 0,
        4, 0, 6, 0);

    //       a1---------->a2
    //                 b1--->b2
    test_segment_intersection<P>("n6",
        2, 0, 6, 0,
        5, 0, 7, 0,
        'c', false, 1, -1,
        5, 0, 6, 0);

    //       a1---------->a2
    //                    b1--->b2
    test_segment_intersection<P>("n7",
        2, 0, 6, 0,
        6, 0, 8, 0,
        'a', false, 0, -1,
        6, 0);

    // Collinear - opposite
    //       a1---------->a2
    // b2<---b1
    test_segment_intersection<P>("o1",
        2, 0, 6, 0,
        2, 0, 0, 0,
        'f', true, -1, -1,
        2, 0);

    //       a1---------->a2
    //    b2<---b1
    test_segment_intersection<P>("o2",
        2, 0, 6, 0,
        3, 0, 1, 0,
        'c', true, -1, -1,
        2, 0, 3, 0);

    //       a1---------->a2
    //       b2<---b1
    test_segment_intersection<P>("o3",
        2, 0, 6, 0,
        4, 0, 2, 0,
        'c', true, -1, 0,
        2, 0, 4, 0);

    //       a1---------->a2
    //           b2<---b1
    test_segment_intersection<P>("o4",
        2, 0, 6, 0,
        5, 0, 3, 0,
        'c', true, -1, 1,
        3, 0, 5, 0);

    //       a1---------->a2
    //              b2<---b1
    test_segment_intersection<P>("o5",
        2, 0, 6, 0,
        6, 0, 4, 0,
        'c', true, 0, 1,
        4, 0, 6, 0);

    //       a1---------->a2
    //                b2<---b1
    test_segment_intersection<P>("o6",
        2, 0, 6, 0,
        7, 0, 5, 0,
        'c', true, 1, 1,
        5, 0, 6, 0);

    //       a1---------->a2
    //                    b2<---b1
    test_segment_intersection<P>("o7",
        2, 0, 6, 0,
        8, 0, 6, 0,
        't', true, 0, 0,
        6, 0);

    //   a1---------->a2
    //   b1---------->b2
    test_segment_intersection<P>("e1",
        2, 0, 6, 0,
        2, 0, 6, 0,
        'e', false, 0, 0,
        2, 0, 6, 0);

    //   a1---------->a2
    //   b2<----------b1
    test_segment_intersection<P>("e1",
        2, 0, 6, 0,
        6, 0, 2, 0,
        'e', true, 0, 0,
        2, 0, 6, 0);

    //   a1---------->a2
    //   b2<----------b1
    test_segment_intersection<P>("case_recursive_boxes_1",
        10, 7, 10, 6,
        10, 10, 10, 9,
        'd', false, 0, 0,
        -1, -1, -1, -1);

}

int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    return 0;
}
