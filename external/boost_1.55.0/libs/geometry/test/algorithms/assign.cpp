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

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/num_points.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename Linestring>
void check_linestring_2d(Linestring const& line)
{
    BOOST_CHECK((boost::size(line) == 3));
    BOOST_CHECK((bg::num_points(line) == 3));

    typedef typename bg::point_type<Linestring>::type point_type;
    point_type const& p0 = line[0];
    BOOST_CHECK(bg::get<0>(p0) == 1);
    BOOST_CHECK(bg::get<1>(p0) == 2);

    point_type const& p1 = line[1];
    BOOST_CHECK(bg::get<0>(p1) == 3);
    BOOST_CHECK(bg::get<1>(p1) == 4);

    point_type const& p2 = line[2];
    BOOST_CHECK(bg::get<0>(p2) == 5);
    BOOST_CHECK(bg::get<1>(p2) == 6);
}

template <typename Point>
void test_assign_linestring_2d()
{
    bg::model::linestring<Point> line;

    // Test assignment of plain array (note that this is only possible if adapted c-array is included!)
    const double coors[3][2] = { {1, 2}, {3, 4}, {5, 6} };
    bg::assign_points(line, coors);
    check_linestring_2d(line);

    // Test assignment of point array
    Point points[3];
    bg::assign_values(points[0], 1, 2);
    bg::assign_values(points[1], 3, 4);
    bg::assign_values(points[2], 5, 6);
    bg::assign_points(line, points);
    check_linestring_2d(line);

    // Test assignment of array with different point-type (tuple adaption should be included)
    boost::tuple<float, float> tuples[3];
    tuples[0] = boost::make_tuple(1, 2);
    tuples[1] = boost::make_tuple(3, 4);
    tuples[2] = boost::make_tuple(5, 6);
    bg::assign_points(line, tuples);
    check_linestring_2d(line);
}

namespace detail
{
    template <typename BoxOrSegment>
    void test_assign_box_or_segment_2d()
    {
        BoxOrSegment geometry;
        bg::assign_values(geometry, 1, 2, 3, 4);
        BOOST_CHECK((bg::get<bg::min_corner, 0>(geometry) == 1));
        BOOST_CHECK((bg::get<bg::min_corner, 1>(geometry) == 2));
        BOOST_CHECK((bg::get<bg::max_corner, 0>(geometry) == 3));
        BOOST_CHECK((bg::get<bg::max_corner, 1>(geometry) == 4));

        bg::assign_zero(geometry);
        BOOST_CHECK((bg::get<bg::min_corner, 0>(geometry) == 0));
        BOOST_CHECK((bg::get<bg::min_corner, 1>(geometry) == 0));
        BOOST_CHECK((bg::get<bg::max_corner, 0>(geometry) == 0));
        BOOST_CHECK((bg::get<bg::max_corner, 1>(geometry) == 0));

        bg::assign_inverse(geometry);
        BOOST_CHECK((bg::get<bg::min_corner, 0>(geometry) > 9999));
        BOOST_CHECK((bg::get<bg::min_corner, 1>(geometry) > 9999));
        BOOST_CHECK((bg::get<bg::max_corner, 0>(geometry) < 9999));
        BOOST_CHECK((bg::get<bg::max_corner, 1>(geometry) < 9999));
    }
}

template <typename Point>
void test_assign_box_or_segment_2d()
{
    detail::test_assign_box_or_segment_2d<bg::model::box<Point> >();
    detail::test_assign_box_or_segment_2d<bg::model::segment<Point> >();
}

template <typename Point>
void test_assign_box_2d()
{
    detail::test_assign_box_or_segment_2d<bg::model::box<Point> >();
}


template <typename Point>
void test_assign_point_3d()
{
    Point p;
    bg::assign_values(p, 1, 2, 3);
    BOOST_CHECK(bg::get<0>(p) == 1);
    BOOST_CHECK(bg::get<1>(p) == 2);
    BOOST_CHECK(bg::get<2>(p) == 3);

    bg::assign_value(p, 123);
    BOOST_CHECK(bg::get<0>(p) == 123);
    BOOST_CHECK(bg::get<1>(p) == 123);
    BOOST_CHECK(bg::get<2>(p) == 123);

    bg::assign_zero(p);
    BOOST_CHECK(bg::get<0>(p) == 0);
    BOOST_CHECK(bg::get<1>(p) == 0);
    BOOST_CHECK(bg::get<2>(p) == 0);

}

template <typename P>
void test_assign_conversion()
{
    typedef bg::model::box<P> box_type;
    typedef bg::model::ring<P> ring_type;
    typedef bg::model::polygon<P> polygon_type;

    P p;
    bg::assign_values(p, 1, 2);

    box_type b;
    bg::assign(b, p);

    BOOST_CHECK_CLOSE((bg::get<0, 0>(b)), 1.0, 0.001);
    BOOST_CHECK_CLOSE((bg::get<0, 1>(b)), 2.0, 0.001);
    BOOST_CHECK_CLOSE((bg::get<1, 0>(b)), 1.0, 0.001);
    BOOST_CHECK_CLOSE((bg::get<1, 1>(b)), 2.0, 0.001);


    bg::set<bg::min_corner, 0>(b, 1);
    bg::set<bg::min_corner, 1>(b, 2);
    bg::set<bg::max_corner, 0>(b, 3);
    bg::set<bg::max_corner, 1>(b, 4);
	
    ring_type ring;
    bg::assign(ring, b);

    {
        typedef bg::model::ring<P, false, false> ring_type_ccw;
        ring_type_ccw ring_ccw;
        // Should NOT compile (currently): bg::assign(ring_ccw, ring);

    }


    //std::cout << bg::wkt(b) << std::endl;
    //std::cout << bg::wkt(ring) << std::endl;

    typename boost::range_const_iterator<ring_type>::type it = ring.begin();
    BOOST_CHECK_CLOSE(bg::get<0>(*it), 1.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(*it), 2.0, 0.001);
    it++;
    BOOST_CHECK_CLOSE(bg::get<0>(*it), 1.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(*it), 4.0, 0.001);
    it++;
    BOOST_CHECK_CLOSE(bg::get<0>(*it), 3.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(*it), 4.0, 0.001);
    it++;
    BOOST_CHECK_CLOSE(bg::get<0>(*it), 3.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(*it), 2.0, 0.001);
    it++;
    BOOST_CHECK_CLOSE(bg::get<0>(*it), 1.0, 0.001);
    BOOST_CHECK_CLOSE(bg::get<1>(*it), 2.0, 0.001);

    BOOST_CHECK_EQUAL(ring.size(), 5u);


    polygon_type polygon;

    bg::assign(polygon, ring);
    BOOST_CHECK_EQUAL(bg::num_points(polygon), 5u);

    ring_type ring2;
    bg::assign(ring2, polygon);
    BOOST_CHECK_EQUAL(bg::num_points(ring2), 5u);
}


template <typename Point>
void test_assign_point_2d()
{
    Point p;
    bg::assign_values(p, 1, 2);
    BOOST_CHECK(bg::get<0>(p) == 1);
    BOOST_CHECK(bg::get<1>(p) == 2);

    bg::assign_value(p, 123);
    BOOST_CHECK(bg::get<0>(p) == 123);
    BOOST_CHECK(bg::get<1>(p) == 123);

    bg::assign_zero(p);
    BOOST_CHECK(bg::get<0>(p) == 0);
    BOOST_CHECK(bg::get<1>(p) == 0);
}





int test_main(int, char* [])
{
    test_assign_point_3d<int[3]>();
    test_assign_point_3d<float[3]>();
    test_assign_point_3d<double[3]>();
    test_assign_point_3d<test::test_point>();
    test_assign_point_3d<bg::model::point<int, 3, bg::cs::cartesian> >();
    test_assign_point_3d<bg::model::point<float, 3, bg::cs::cartesian> >();
    test_assign_point_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

    test_assign_point_2d<int[2]>();
    test_assign_point_2d<float[2]>();
    test_assign_point_2d<double[2]>();
    test_assign_point_2d<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_assign_point_2d<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_assign_point_2d<bg::model::point<double, 2, bg::cs::cartesian> >();

    test_assign_conversion<bg::model::point<double, 2, bg::cs::cartesian> >();


    // Segment (currently) cannot handle array's because derived from std::pair
    test_assign_box_2d<int[2]>();
    test_assign_box_2d<float[2]>();
    test_assign_box_2d<double[2]>();

    test_assign_box_or_segment_2d<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_assign_box_or_segment_2d<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_assign_box_or_segment_2d<bg::model::point<double, 2, bg::cs::cartesian> >();

    test_assign_linestring_2d<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_assign_linestring_2d<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_assign_linestring_2d<bg::model::point<double, 2, bg::cs::cartesian> >();

#ifdef HAVE_TTMATH
    // Next 3 need extra traits for c-array with custom type:
    // test_assign_point_2d<ttmath_big[2]>();
    // test_assign_point_3d<ttmath_big[3]>();
    // test_assign_box_2d<ttmath_big[2]>();

    test_assign_point_2d<bg::model::point<ttmath_big, 2, bg::cs::cartesian> >();
    test_assign_point_3d<bg::model::point<ttmath_big, 3, bg::cs::cartesian> >();
    test_assign_box_or_segment_2d<bg::model::point<ttmath_big, 2, bg::cs::cartesian> >();
    test_assign_linestring_2d<bg::model::point<ttmath_big, 2, bg::cs::cartesian> >();
#endif

    return 0;
}
