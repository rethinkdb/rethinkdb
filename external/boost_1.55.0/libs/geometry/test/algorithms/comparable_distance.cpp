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


#include <sstream>

#include <boost/mpl/if.hpp>
#include <geometry_test_common.hpp>

#include <boost/geometry/geometry.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>

#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/strategies/strategies.hpp>


#include <boost/geometry/geometries/geometries.hpp>


template <typename P>
void test_distance_result()
{
    typedef typename bg::default_distance_result<P, P>::type distance_type;

    P p1 = bg::make<P>(0, 0);
    P p2 = bg::make<P>(3, 0);
    P p3 = bg::make<P>(0, 4);

    distance_type dr12 = bg::comparable_distance(p1, p2);
    distance_type dr13 = bg::comparable_distance(p1, p3);
    distance_type dr23 = bg::comparable_distance(p2, p3);

    BOOST_CHECK_CLOSE(dr12, 9.000, 0.001);
    BOOST_CHECK_CLOSE(dr13, 16.000, 0.001);
    BOOST_CHECK_CLOSE(dr23, 25.000, 0.001);

}

template <typename P>
void test_distance_point()
{
    P p1;
    bg::set<0>(p1, 1);
    bg::set<1>(p1, 1);

    P p2;
    bg::set<0>(p2, 2);
    bg::set<1>(p2, 2);

    typename bg::coordinate_type<P>::type d = bg::comparable_distance(p1, p2);
    BOOST_CHECK_CLOSE(d, 2.0, 0.001);
}

template <typename P>
void test_distance_segment()
{
    typedef typename bg::coordinate_type<P>::type coordinate_type;

    P s1 = bg::make<P>(2, 2);
    P s2 = bg::make<P>(3, 3);

    // Check points left, right, projected-left, projected-right, on segment
    P p1 = bg::make<P>(0, 0);
    P p2 = bg::make<P>(4, 4);
    P p3 = bg::make<P>(2.4, 2.6);
    P p4 = bg::make<P>(2.6, 2.4);
    P p5 = bg::make<P>(2.5, 2.5);

    bg::model::referring_segment<P const> const seg(s1, s2);

    coordinate_type d1 = bg::comparable_distance(p1, seg); BOOST_CHECK_CLOSE(d1, 8.0, 0.001);
    coordinate_type d2 = bg::comparable_distance(p2, seg); BOOST_CHECK_CLOSE(d2, 2.0, 0.001);
    coordinate_type d3 = bg::comparable_distance(p3, seg); BOOST_CHECK_CLOSE(d3, 0.02, 0.001);
    coordinate_type d4 = bg::comparable_distance(p4, seg); BOOST_CHECK_CLOSE(d4, 0.02, 0.001);
    coordinate_type d5 = bg::comparable_distance(p5, seg); BOOST_CHECK_CLOSE(d5, 0.0, 0.001);

    // Reverse case
    coordinate_type dr1 = bg::comparable_distance(seg, p1); BOOST_CHECK_CLOSE(dr1, d1, 0.001);
    coordinate_type dr2 = bg::comparable_distance(seg, p2); BOOST_CHECK_CLOSE(dr2, d2, 0.001);
}

template <typename P>
void test_distance_linestring()
{
    bg::model::linestring<P> points;
    points.push_back(bg::make<P>(1, 1));
    points.push_back(bg::make<P>(3, 3));

    P p = bg::make<P>(2, 1);

    typename bg::coordinate_type<P>::type d = bg::comparable_distance(p, points);
    BOOST_CHECK_CLOSE(d, 0.5, 0.001);

    p = bg::make<P>(5, 5);
    d = bg::comparable_distance(p, points);
    BOOST_CHECK_CLOSE(d, 8.0, 0.001);


    bg::model::linestring<P> line;
    line.push_back(bg::make<P>(1,1));
    line.push_back(bg::make<P>(2,2));
    line.push_back(bg::make<P>(3,3));

    p = bg::make<P>(5, 5);

    d = bg::comparable_distance(p, line);
    BOOST_CHECK_CLOSE(d, 8.0, 0.001);

    // Reverse case
    d = bg::comparable_distance(line, p);
    BOOST_CHECK_CLOSE(d, 8.0, 0.001);
}

template <typename P>
void test_all()
{
    test_distance_result<P>();
    test_distance_point<P>();
    test_distance_segment<P>();
    test_distance_linestring<P>();
}

int test_main(int, char* [])
{
    //test_all<bg::model::d2::point_xy<int> >();
    test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

#ifdef HAVE_TTMATH
    test_all<bg::model::d2::point_xy<ttmath_big> >();
#endif
    return 0;
}
