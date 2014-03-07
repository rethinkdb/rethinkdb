// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithms/test_covered_by.hpp>


#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>


template <typename P>
void test_all()
{
    /*
    // trivial case
    test_ring<P>("POINT(1 1)", "POLYGON((0 0,0 2,2 2,2 0,0 0))", true, false);

    // on border/corner
    test_ring<P>("POINT(0 0)", "POLYGON((0 0,0 2,2 2,2 0,0 0))", false, true);
    test_ring<P>("POINT(0 1)", "POLYGON((0 0,0 2,2 2,2 0,0 0))", false, true);

    // aligned to segment/vertex
    test_ring<P>("POINT(1 1)", "POLYGON((0 0,0 3,3 3,3 1,2 1,2 0,0 0))", true, false);
    test_ring<P>("POINT(1 1)", "POLYGON((0 0,0 3,4 3,3 1,2 2,2 0,0 0))", true, false);

    // same polygon, but point on border
    test_ring<P>("POINT(3 3)", "POLYGON((0 0,0 3,3 3,3 1,2 1,2 0,0 0))", false, true);
    test_ring<P>("POINT(3 3)", "POLYGON((0 0,0 3,4 3,3 1,2 2,2 0,0 0))", false, true);

    // holes
    test_geometry<P, bg::model::polygon<P> >("POINT(2 2)",
        "POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,3 1,3 3,1 3,1 1))", false);

    */

    typedef bg::model::box<P> box_type;

    test_geometry<P, box_type>("POINT(1 1)", "BOX(0 0,2 2)", true);
    test_geometry<P, box_type>("POINT(0 0)", "BOX(0 0,2 2)", true);
    test_geometry<P, box_type>("POINT(2 2)", "BOX(0 0,2 2)", true);
    test_geometry<P, box_type>("POINT(0 1)", "BOX(0 0,2 2)", true);
    test_geometry<P, box_type>("POINT(1 0)", "BOX(0 0,2 2)", true);
    test_geometry<P, box_type>("POINT(3 3)", "BOX(0 0,2 2)", false);

    test_geometry<box_type, box_type>("BOX(1 1,2 2)", "BOX(0 0,3 3)", true);
    test_geometry<box_type, box_type>("BOX(0 0,3 3)", "BOX(1 1,2 2)", false);
    test_geometry<box_type, box_type>("BOX(0 0,2 2)", "BOX(0 0,3 3)", true);
    test_geometry<box_type, box_type>("BOX(1 1,3 3)", "BOX(0 0,3 3)", true);
    test_geometry<box_type, box_type>("BOX(1 2,3 3)", "BOX(0 0,3 3)", true);
    test_geometry<box_type, box_type>("BOX(1 1,4 3)", "BOX(0 0,3 3)", false);


    /*
    test_within_code<P, box_type>("POINT(1 1)", "BOX(0 0,2 2)", 1);
    test_within_code<P, box_type>("POINT(1 0)", "BOX(0 0,2 2)", 0);
    test_within_code<P, box_type>("POINT(0 1)", "BOX(0 0,2 2)", 0);
    test_within_code<P, box_type>("POINT(0 3)", "BOX(0 0,2 2)", -1);
    test_within_code<P, box_type>("POINT(3 3)", "BOX(0 0,2 2)", -1);

    test_within_code<box_type, box_type>("BOX(1 1,2 2)", "BOX(0 0,3 3)", 1);
    test_within_code<box_type, box_type>("BOX(0 1,2 2)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 0,2 2)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 1,2 3)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 1,3 2)", "BOX(0 0,3 3)", 0);
    test_within_code<box_type, box_type>("BOX(1 1,3 4)", "BOX(0 0,3 3)", -1);
    */
}


void test_3d()
{
    typedef boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian> point_type;
    typedef boost::geometry::model::box<point_type> box_type;
    box_type box(point_type(0, 0, 0), point_type(4, 4, 4));
    BOOST_CHECK_EQUAL(bg::covered_by(point_type(2, 2, 2), box), true);
    BOOST_CHECK_EQUAL(bg::covered_by(point_type(2, 4, 2), box), true);
    BOOST_CHECK_EQUAL(bg::covered_by(point_type(2, 2, 4), box), true);
    BOOST_CHECK_EQUAL(bg::covered_by(point_type(2, 2, 5), box), false);
}

template <typename P1, typename P2>
void test_mixed_of()
{
    typedef boost::geometry::model::polygon<P1> polygon_type1;
    typedef boost::geometry::model::polygon<P2> polygon_type2;
    typedef boost::geometry::model::box<P1> box_type1;
    typedef boost::geometry::model::box<P2> box_type2;

    polygon_type1 poly1, poly2;
    boost::geometry::read_wkt("POLYGON((0 0,0 5,5 5,5 0,0 0))", poly1);
    boost::geometry::read_wkt("POLYGON((0 0,0 5,5 5,5 0,0 0))", poly2);

    box_type1 box1(P1(1, 1), P1(4, 4));
    box_type2 box2(P2(0, 0), P2(5, 5));
    P1 p1(3, 3);
    P2 p2(3, 3);

    BOOST_CHECK_EQUAL(bg::covered_by(p1, poly2), true);
    BOOST_CHECK_EQUAL(bg::covered_by(p2, poly1), true);
    BOOST_CHECK_EQUAL(bg::covered_by(p2, box1), true);
    BOOST_CHECK_EQUAL(bg::covered_by(p1, box2), true);
    BOOST_CHECK_EQUAL(bg::covered_by(box1, box2), true);
    BOOST_CHECK_EQUAL(bg::covered_by(box2, box1), false);
}


void test_mixed()
{
    // Mixing point types and coordinate types
    test_mixed_of
        <
            boost::geometry::model::d2::point_xy<double>,
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        >();
    test_mixed_of
        <
            boost::geometry::model::d2::point_xy<float>,
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        >();
    test_mixed_of
        <
            boost::geometry::model::d2::point_xy<int>,
            boost::geometry::model::d2::point_xy<double>
        >();
}


int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<double> >();

    //test_spherical<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> > >();

    test_mixed();
    test_3d();


#if defined(HAVE_TTMATH)
    test_all<bg::model::d2::point_xy<ttmath_big> >();
    //test_spherical<bg::model::point<ttmath_big, 2, bg::cs::spherical_equatorial<bg::degree> > >();
#endif

    return 0;
}
