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


#if defined(_MSC_VER)
// We deliberately mix float/double's here so turn off warning
#pragma warning( disable : 4244 )
#endif // defined(_MSC_VER)

#define HAVE_MATRIX_AS_STRING


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/assign.hpp>

#include <boost/geometry/strategies/cartesian/cart_intersect.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>

#include <boost/geometry/policies/relate/intersection_points.hpp>
#include <boost/geometry/policies/relate/direction.hpp>
//#include <boost/geometry/policies/relate/de9im.hpp>
#include <boost/geometry/policies/relate/tupled.hpp>

#include <boost/geometry/algorithms/intersection.hpp>


#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian);


template <typename R>
void print_is(R const& is)
{
#ifdef REPORT
    for (int i = 0; i < is.count; i++)
    {
        std::cout
            << " (" << bg::get<0>(is.intersections[i])
            << "," << bg::get<1>(is.intersections[i])
            << ")";
    }
#endif
}
/*
void print_im(bg::de9im const& im)
{
#ifdef REPORT
    if (im.equals()) std::cout << " EQUALS";
    if (im.disjoint()) std::cout << " DISJOINT";
    if (im.intersects()) std::cout << " INTERSECTS";
    if (im.touches()) std::cout << " TOUCHES";
    if (im.crosses()) std::cout << " CROSSES";
    if (im.overlaps()) std::cout << " OVERLAPS";
    if (im.within()) std::cout << " WITHIN";
    if (im.contains()) std::cout << " CONTAINS";

    //std::cout << " ra=" << im.ra << " rb=" << im.rb;
#endif
}
*/

template <typename P>
static void test_segment_intersection(int caseno,
                int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
                std::string const& expected_matrix,
                std::string const& expected_characteristic,
                std::string const& expected_direction_a = "",
                std::string const& expected_direction_b = "",
                int expected_x1 = -99, int expected_y1 = -99,
                int expected_x2 = -99, int expected_y2 = -99)
{
    using namespace boost::geometry;

#ifdef REPORT
    std::cout << "CASE " << caseno << std::endl;
#endif

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
    segment_intersection_points<P> is
        = strategy::intersection::relate_cartesian_segments
        <
            policies::relate::segments_intersection_points
                <
                    segment_type,
                    segment_type,
                    segment_intersection_points<P>
                >
        >::apply(s12, s34);

    // Get the Dimension Extended 9 Intersection Matrix (de9im) for Segments
    // (this one is extended by GGL having opposite)
    /***
    TODO TO BE UPDATED (if necessary) OR DELETED
    de9im_segment matrix = strategy::intersection::relate_cartesian_segments
        <
            policies::relate::segments_de9im<segment_type, segment_type>
        >::apply(s12, s34);
    ***/

    // Get just a character for Left/Right/intersects/etc, purpose is more for debugging
    policies::relate::direction_type dir = strategy::intersection::relate_cartesian_segments
        <
            policies::relate::segments_direction<segment_type, segment_type>
        >::apply(s12, s34);

    int expected_count = 0;

    if (expected_x1 != -99 && expected_y1 != -99)
    {
        expected_count++;

        BOOST_CHECK(is.count >= 1);
        BOOST_CHECK_CLOSE(bg::get<0>(is.intersections[0]),
                coordinate_type(expected_x1), 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(is.intersections[0]),
                coordinate_type(expected_y1), 0.001);
    }
    if (expected_x2 != -99 && expected_y2 != -99)
    {
        expected_count++;

        BOOST_CHECK(is.count >= 2);
        BOOST_CHECK_CLOSE(bg::get<0>(is.intersections[1]),
                coordinate_type(expected_x2), 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(is.intersections[1]),
                coordinate_type(expected_y2), 0.001);
    }
    BOOST_CHECK_EQUAL(is.count, expected_count);

    //BOOST_CHECK_EQUAL(expected_matrix, matrix.matrix_as_string());

    std::string characteristic;
    characteristic += dir.how;

    BOOST_CHECK_EQUAL(characteristic, expected_characteristic);

    if (!expected_direction_a.empty())
    {
        BOOST_CHECK_EQUAL(dir.dir_a == 1 ? "L"
                    : dir.dir_a == -1 ? "R"
                    : "-", expected_direction_a);
    }
    if (!expected_direction_b.empty())
    {
        BOOST_CHECK_EQUAL(dir.dir_b == 1 ? "L"
                    : dir.dir_b == -1 ? "R"
                    : "-", expected_direction_b);
    }



    // Things can also be used together
    // -> intersection is only once calculated, two results
    /***
    typedef boost::tuple
        <
            de9im_segment,
            policies::relate::direction_type
        > tup;

    tup t = strategy::intersection::relate_cartesian_segments
        <
            policies::relate::segments_tupled
                <
                    policies::relate::segments_de9im<segment_type, segment_type>,
                    policies::relate::segments_direction<segment_type, segment_type>
                >
        >::apply(segment_type(p1,p2), segment_type(p3,p4));


    //BOOST_CHECK_EQUAL(t.get<0>().matrix_as_string(), matrix.matrix_as_string());
    BOOST_CHECK_EQUAL(t.get<1>().how, dir.how);
    */

#ifdef REPORT
    //std::cout << std::endl << "\t" << matrix.as_string() << " ";
    std::cout << "METHOD=" << c << " ";
    print_is(is);
    //print_im(matrix);
    std::cout << std::endl;
#endif

    /*
    To check with a spatial database: issue this statement
    std::cout << "select relate("
            << "GeomFromText(LINESTRING('" << x1 << " " << y1 << "," << x2 << " " << y2 << ")')"
            << ", "
            << "GeomFromText(LINESTRING('" << x3 << " " << y3 << "," << x4 << " " << y4 << ")')"
            << ");"
            << std::endl;
    */


    // Now use generic intersection.
    std::vector<P> out;
    bg::detail::intersection::intersection_insert<P>(s12, s34, std::back_inserter(out));

    BOOST_CHECK_EQUAL(boost::size(out), expected_count);

    if (expected_x1 != -99 && expected_y1 != -99
        && is.count >= 1
        && boost::size(out) >= 1)
    {
        BOOST_CHECK_CLOSE(bg::get<0>(out[0]),
                coordinate_type(expected_x1), 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(out[0]),
                coordinate_type(expected_y1), 0.001);
    }
    if (expected_x2 != -99 && expected_y2 != -99
        && is.count >= 2
        && boost::size(out) >= 2)
    {
        BOOST_CHECK_CLOSE(bg::get<0>(out[1]),
                coordinate_type(expected_x2), 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(out[1]),
                coordinate_type(expected_y2), 0.001);
    }
}


template <typename P>
void test_all()
{
    test_segment_intersection<P>( 1, 0,2, 2,0, 0,0, 2,2, "0-1--0102",
        "i", "R", "L", 1, 1);
    test_segment_intersection<P>( 2, 2,2, 3,1, 0,0, 2,2, "--1-00102",
        "a", "R", "R", 2, 2);
    test_segment_intersection<P>( 3, 3,1, 2,2, 0,0, 2,2, "--1-00102",
        "t", "R", "L", 2, 2);
    test_segment_intersection<P>( 4, 0,2, 1,1, 0,0, 2,2, "--10-0102",
        "m", "L", "L", 1, 1);
#ifdef REPORT
    std::cout << std::endl;
#endif

    test_segment_intersection<P>( 5, 1,1, 0,2, 0,0, 2,2, "--10-0102",
        "s", "L", "R", 1, 1);
    test_segment_intersection<P>( 6, 0,2, 2,0, 0,0, 1,1, "-01--0102",
        "m", "R", "R", 1, 1);
    test_segment_intersection<P>( 7, 2,0, 0,2, 0,0, 1,1, "-01--0102",
        "m", "L", "L", 1, 1);
    test_segment_intersection<P>( 8, 2,3, 3,2, 0,0, 2,2, "--1--0102",
        "d");
#ifdef REPORT
    std::cout << std::endl;
#endif

    test_segment_intersection<P>( 9, 0,0, 2,2, 0,0, 2,2, "1---0---2",
        "e", "-", "-", 0, 0, 2, 2);
    test_segment_intersection<P>(10, 2,2, 0,0, 0,0, 2,2, "1---0---2",
        "e", "-", "-", 2, 2, 0, 0);
    test_segment_intersection<P>(11, 1,1, 3,3, 0,0, 2,2, "1010-0102",
        "c", "-", "-", 1, 1, 2, 2);
    test_segment_intersection<P>(12, 3,3, 1,1, 0,0, 2,2, "1010-0102",
        "c", "-", "-", 1, 1, 2, 2);
#ifdef REPORT
    std::cout << std::endl;
#endif

    test_segment_intersection<P>(13, 0,2, 2,2, 2,1, 2,3, "--10-0102",
        "m", "L", "L", 2, 2);
    test_segment_intersection<P>(14, 2,2, 2,4, 2,0, 2,2, "--1-00102",
        "C", "-", "-", 2, 2);
    test_segment_intersection<P>(15, 2,2, 2,4, 2,0, 2,1, "--1--0102",
        "d");
    test_segment_intersection<P>(16, 2,4, 2,2, 2,0, 2,1, "--1--0102",
        "d");

    test_segment_intersection<P>(17, 2,1, 2,3, 2,2, 2,4, "1010-0102",
        "c", "-", "-", 2, 3, 2, 2);
    test_segment_intersection<P>(18, 2,3, 2,1, 2,2, 2,4, "1010-0102",
        "c", "-", "-", 2, 3, 2, 2);
    test_segment_intersection<P>(19, 0,2, 2,2, 4,2, 2,2, "--1-00102",
        "C", "-", "-", 2, 2);
    test_segment_intersection<P>(20, 0,2, 2,2, 2,2, 4,2, "--1-00102",
        "C", "-", "-", 2, 2);

    test_segment_intersection<P>(21, 1,2, 3,2, 2,1, 2,3, "0-1--0102",
        "i", "R", "L", 2, 2);
    test_segment_intersection<P>(22, 2,4, 2,1, 2,1, 2,3, "101-00--2",
        "c", "-", "-", 2, 1, 2, 3);
    test_segment_intersection<P>(23, 2,4, 2,1, 2,3, 2,1, "101-00--2",
        "c", "-", "-", 2, 3, 2, 1);
    test_segment_intersection<P>(24, 1,1, 3,3, 0,0, 3,3, "1--00-102",
        "c", "-", "-", 1, 1, 3, 3);

    test_segment_intersection<P>(25, 2,0, 2,4, 2,1, 2,3, "101--0--2",
        "c", "-", "-", 2, 1, 2, 3);
    test_segment_intersection<P>(26, 2,0, 2,4, 2,3, 2,1, "101--0--2",
        "c", "-", "-", 2, 3, 2, 1);
    test_segment_intersection<P>(27, 0,0, 4,4, 1,1, 3,3, "101--0--2",
        "c", "-", "-", 1, 1, 3, 3);
    test_segment_intersection<P>(28, 0,0, 4,4, 3,3, 1,1, "101--0--2",
        "c", "-", "-", 3, 3, 1, 1);

    test_segment_intersection<P>(29, 1,1, 3,3, 0,0, 4,4, "1--0--102",
        "c", "-", "-", 1, 1, 3, 3);
    test_segment_intersection<P>(30, 0,0, 2,2, 2,2, 3,1, "--1-00102",
        "a", "R", "R", 2, 2);
    test_segment_intersection<P>(31, 0,0, 2,2, 2,2, 1,3, "--1-00102",
        "a", "L", "L", 2, 2);
    test_segment_intersection<P>(32, 0,0, 2,2, 1,1, 2,0, "-01--0102",
        "s", "L", "R", 1, 1);

    test_segment_intersection<P>(33, 0,0, 2,2, 1,1, 0,2, "-01--0102",
        "s", "R", "L", 1, 1);
    test_segment_intersection<P>(34, 2,2, 1,3, 0,0, 2,2, "--1-00102",
        "a", "L", "L", 2, 2);
    test_segment_intersection<P>(35, 2,2, 3,1, 0,0, 2,2, "--1-00102",
        "a", "R", "R", 2, 2);
    test_segment_intersection<P>(36, 0,0, 2,2, 0,2, 1,1, "-01--0102",
        "m", "L", "L", 1, 1);

    test_segment_intersection<P>(37, 0,0, 2,2, 2,0, 1,1, "-01--0102",
        "m", "R", "R", 1, 1);
    test_segment_intersection<P>(38, 1,1, 0,2, 0,0, 2,2, "--10-0102",
        "s", "L", "R", 1, 1);
    test_segment_intersection<P>(39, 1,1, 2,0, 0,0, 2,2, "--10-0102",
        "s", "R", "L", 1, 1);
    test_segment_intersection<P>(40, 2,0, 1,1, 0,0, 2,2, "--10-0102",
        "m", "R", "R", 1, 1);

    test_segment_intersection<P>(41, 1,2, 0,2, 2,2, 0,2, "1--00-102",
        "c", "-", "-", 1, 2, 0, 2);
    test_segment_intersection<P>(42, 2,1, 1,1, 2,2, 0,2, "--1--0102",
        "p");
    test_segment_intersection<P>(43, 4,1, 3,1, 2,2, 0,2, "--1--0102",
        "p");
    test_segment_intersection<P>(44, 4,2, 3,2, 2,2, 0,2, "--1--0102",
        "d");

    test_segment_intersection<P>(45, 2,0, 0,2, 0,0, 2,2, "0-1--0102",
        "i", "L", "R", 1, 1);

    // In figure: times 2
    test_segment_intersection<P>(46, 8,2, 4,6, 0,0, 8, 8, "0-1--0102",
        "i", "L", "R", 5, 5);
}

int test_main(int, char* [])
{
    std::cout << "Note this test is out-of-date and either obsolete or should be updated" << std::endl;
    test_all<boost::tuple<double, double> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    return 0;
}
