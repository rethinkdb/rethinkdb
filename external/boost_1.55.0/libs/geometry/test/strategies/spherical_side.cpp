// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>


#include <boost/geometry/algorithms/assign.hpp>


#include <boost/geometry/strategies/spherical/side_by_cross_track.hpp>
//#include <boost/geometry/strategies/spherical/side_via_plane.hpp>
#include <boost/geometry/strategies/spherical/ssf.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>

#include <boost/geometry/core/cs.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry {

template <typename Vector, typename Point1, typename Point2>
static inline Vector create_vector(Point1 const& p1, Point2 const& p2)
{
    Vector v;
    convert(p1, v);
    subtract_point(v, p2);
    return v;
}

}}

inline char side_char(int side)
{
    return side == 1 ? 'L'
        : side == -1 ? 'R'
        : '-'
        ;
}

template <typename Point>
void test_side1(std::string const& case_id, Point const& p1, Point const& p2, Point const& p3,
                   int expected, int expected_cartesian)
{
    // std::cout << case_id << ": ";
    //int s = bg::strategy::side::side_via_plane<>::apply(p1, p2, p3);
    int side_ssf = bg::strategy::side::spherical_side_formula<>::apply(p1, p2, p3);
    //int side2 = bg::strategy::side::side_via_plane<>::apply(p1, p2, p3);
    int side_ct = bg::strategy::side::side_by_cross_track<>::apply(p1, p2, p3);

    typedef bg::strategy::side::services::default_strategy<bg::cartesian_tag>::type cartesian_strategy;
    int side_cart = cartesian_strategy::apply(p1, p2, p3);


    BOOST_CHECK_EQUAL(side_ssf, expected);
    BOOST_CHECK_EQUAL(side_ct, expected);
    BOOST_CHECK_EQUAL(side_cart, expected_cartesian);
    /*
    std::cout 
        << "exp: " << side_char(expected)
        << " ssf: " << side_char(side1)
        << " pln: " << side_char(side2)
        << " ct: " << side_char(side3)
        //<< " def: " << side_char(side4)
        << " cart: " << side_char(side5)
        << std::endl;
    */
}

template <typename Point>
void test_side(std::string const& case_id, Point const& p1, Point const& p2, Point const& p3,
                   int expected, int expected_cartesian = -999)
{
    if (expected_cartesian == -999)
    {
        expected_cartesian = expected;
    }
    test_side1(case_id, p1, p2, p3, expected, expected_cartesian);
    test_side1(case_id, p2, p1, p3, -expected, -expected_cartesian);
}


template <typename Point>
void test_all()
{
    typedef std::pair<double, double> pair;

    Point amsterdam(5.9, 52.4);
    Point barcelona(2.0, 41.0);
    Point paris(2.0, 48.0);
    Point milan(7.0, 45.0);

    //goto wrong;

    test_side<Point>("bp-m", barcelona, paris, milan, -1);
    test_side<Point>("bm-p", barcelona, milan, paris, 1);
    test_side<Point>("mp-b", milan, paris, barcelona, 1);

    test_side<Point>("am-p", amsterdam, milan, paris, -1);
    test_side<Point>("pm-a", paris, milan, amsterdam, 1);

    // http://www.gcmap.com/mapui?P=30N+10E-50N+50E,39N+30E
    Point gcmap_p1(10.0, 30.0);
    Point gcmap_p2(50.0, 50.0);
    test_side<Point>("blog1", gcmap_p1, gcmap_p2, Point(30.0, 41.0), -1, 1);
    test_side<Point>("blog1", gcmap_p1, gcmap_p2, Point(30.0, 42.0), -1, 1);
    test_side<Point>("blog1", gcmap_p1, gcmap_p2, Point(30.0, 43.0), -1, 1);
    test_side<Point>("blog1", gcmap_p1, gcmap_p2, Point(30.0, 44.0), 1);

    // http://www.gcmap.com/mapui?P=50N+80E-60N+50W,65N+30E
    Point gcmap_np1(80.0, 50.0);
    Point gcmap_np2(-50.0, 60.0);
    // http://www.gcmap.com/mapui?P=50N+140E-60N+10E,65N+30E
    //Point gcmap_np1(140.0, 50.0);
    //Point gcmap_np2(10.0, 60.0);
    //test_side<Point>(gcmap_np1, gcmap_np2, gcmap_np, 1);
    test_side<Point>("40", gcmap_np1, gcmap_np2, Point(30.0, 60.0), 1, -1);
    test_side<Point>("45", gcmap_np1, gcmap_np2, Point(30.0, 65.0), 1, -1);
    test_side<Point>("70", gcmap_np1, gcmap_np2, Point(30.0, 70.0), 1, -1);
    test_side<Point>("75", gcmap_np1, gcmap_np2, Point(30.0, 75.0), -1);
}

int test_main(int, char* [])
{
    test_all<bg::model::point<int, 2, bg::cs::spherical<bg::degree> > >();
    test_all<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> > >();

#if defined(HAVE_TTMATH)
    typedef ttmath::Big<1,4> tt;
    test_all<bg::model::point<tt, 2, bg::cs::spherical_equatorial<bg::degree> > >();
#endif

    return 0;
}
