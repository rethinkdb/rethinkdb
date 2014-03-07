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

#include <algorithms/test_expand.hpp>


#include <boost/geometry/algorithms/make.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/adapted/std_pair_as_segment.hpp>
#include <test_common/test_point.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename Point>
void test_point_3d()
{
    bg::model::box<Point> b = bg::make_inverse<bg::model::box<Point> >();

    test_expand<Point>(b, "POINT(1 2 5)", "(1,2,5),(1,2,5)");
    test_expand<Point>(b, "POINT(3 4 6)", "(1,2,5),(3,4,6)");

    test_expand<Point>(b, "POINT(4 4 5)", "(1,2,5),(4,4,6)");
    test_expand<Point>(b, "POINT(4 5 5)", "(1,2,5),(4,5,6)");
    test_expand<Point>(b, "POINT(10 10 4)", "(1,2,4),(10,10,6)");
    test_expand<Point>(b, "POINT(9 9 4)", "(1,2,4),(10,10,6)");

    test_expand<Point>(b, "POINT(0 2 7)", "(0,2,4),(10,10,7)");
    test_expand<Point>(b, "POINT(0 0 7)", "(0,0,4),(10,10,7)");
    test_expand<Point>(b, "POINT(-1 -1 5)", "(-1,-1,4),(10,10,7)");
    test_expand<Point>(b, "POINT(0 0 5)", "(-1,-1,4),(10,10,7)");

    test_expand<Point>(b, "POINT(15 -1 0)", "(-1,-1,0),(15,10,7)");
    test_expand<Point>(b, "POINT(-1 15 10)", "(-1,-1,0),(15,15,10)");
}

template <typename Point>
void test_box_3d()
{
    typedef bg::model::box<Point> box_type;
    box_type b = bg::make_inverse<box_type>();

    test_expand<box_type>(b, "BOX(0 2 5,4 4 6)",   "(0,2,5),(4,4,6)");
    test_expand<box_type>(b, "BOX(0 1 5,4 6 6)",   "(0,1,5),(4,6,6)");
    test_expand<box_type>(b, "BOX(-1 -1 6,10 10 5)", "(-1,-1,5),(10,10,6)");
    test_expand<box_type>(b, "BOX(3 3 6,3 3 5)",   "(-1,-1,5),(10,10,6)");

    test_expand<box_type>(b, "BOX(3 15 7,-1 3 4)", "(-1,-1,4),(10,15,7)");
    test_expand<box_type>(b, "BOX(-15 3 7,3 20 4)", "(-15,-1,4),(10,20,7)");
    test_expand<box_type>(b, "BOX(3 -20 8,3 20 3)", "(-15,-20,3),(10,20,8)");
    test_expand<box_type>(b, "BOX(-20 3 8,20 3 3)", "(-20,-20,3),(20,20,8)");
}



template <typename P>
void test_3d()
{
    test_point_3d<P>();
    test_box_3d<P>();
}

template <typename Point>
void test_2d()
{
    typedef bg::model::box<Point> box_type;
    typedef std::pair<Point, Point> segment_type;

    box_type b = bg::make_inverse<box_type>();

    test_expand<box_type>(b, "BOX(1 1,2 2)",   "(1,1),(2,2)");

    // Test an 'incorrect' box -> should also correctly update the bbox
    test_expand<box_type>(b, "BOX(3 4,0 1)",   "(0,1),(3,4)");

    // Test a segment
    test_expand<segment_type>(b, "SEGMENT(5 6,7 8)",   "(0,1),(7,8)");
}

template <typename Point>
void test_spherical_degree()
{
    bg::model::box<Point> b = bg::make_inverse<bg::model::box<Point> >();

    test_expand<Point>(b, "POINT(179.73 71.56)",
            "(179.73,71.56),(179.73,71.56)");
    test_expand<Point>(b, "POINT(177.47 71.23)",
            "(177.47,71.23),(179.73,71.56)");

    // It detects that this point is lying RIGHT of the others,
    //      and then it "expands" it.
    // It might be argued that "181.22" is displayed instead. However, they are
    // the same.
    test_expand<Point>(b, "POINT(-178.78 70.78)",
            "(177.47,70.78),(-178.78,71.56)");
}


template <typename Point>
void test_spherical_radian()
{
    bg::model::box<Point> b = bg::make_inverse<bg::model::box<Point> >();

    test_expand<Point>(b, "POINT(3.128 1.249)",
            "(3.128,1.249),(3.128,1.249)");
    test_expand<Point>(b, "POINT(3.097 1.243)",
            "(3.097,1.243),(3.128,1.249)");

    // It detects that this point is lying RIGHT of the others,
    //      and then it "expands" it.
    // It might be argued that "181.22" is displayed instead. However, they are
    // the same.
    test_expand<Point>(b, "POINT(-3.121 1.235)",
            "(3.097,1.235),(-3.121,1.249)");
}

int test_main(int, char* [])
{
    test_2d<bg::model::point<int, 2, bg::cs::cartesian> >();


    test_3d<test::test_point>();
    test_3d<bg::model::point<int, 3, bg::cs::cartesian> >();
    test_3d<bg::model::point<float, 3, bg::cs::cartesian> >();
    test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

    test_spherical_degree<bg::model::point<double, 2, bg::cs::spherical<bg::degree> > >();
    test_spherical_radian<bg::model::point<double, 2, bg::cs::spherical<bg::radian> > >();


#if defined(HAVE_TTMATH)
    test_3d<bg::model::point<ttmath_big, 3, bg::cs::cartesian> >();
    test_spherical_degree<bg::model::point<ttmath_big, 2, bg::cs::spherical<bg::degree> > >();
    test_spherical_radian<bg::model::point<ttmath_big, 2, bg::cs::spherical<bg::radian> > >();
#endif

    return 0;
}
