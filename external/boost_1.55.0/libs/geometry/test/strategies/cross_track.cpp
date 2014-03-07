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

#include <boost/geometry/io/wkt/read.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/distance.hpp>

#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
#include <boost/geometry/strategies/spherical/distance_cross_track.hpp>

#include <boost/geometry/strategies/concepts/distance_concept.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>


// This test is GIS oriented. 


template <typename Point, typename LatitudePolicy>
void test_distance(
            typename bg::coordinate_type<Point>::type const& lon1, 
            typename bg::coordinate_type<Point>::type const& lat1,
            typename bg::coordinate_type<Point>::type const& lon2, 
            typename bg::coordinate_type<Point>::type const& lat2,
            typename bg::coordinate_type<Point>::type const& lon3, 
            typename bg::coordinate_type<Point>::type const& lat3,
            typename bg::coordinate_type<Point>::type const& radius, 
            typename bg::coordinate_type<Point>::type const& expected, 
            typename bg::coordinate_type<Point>::type const& tolerance)
{
    typedef bg::strategy::distance::cross_track
        <
            typename bg::coordinate_type<Point>::type
        > strategy_type;

    typedef typename bg::strategy::distance::services::return_type
        <
            strategy_type,
            Point,
            Point
        >::type return_type;


    BOOST_CONCEPT_ASSERT
        (
            (bg::concept::PointSegmentDistanceStrategy<strategy_type, Point, Point>)
        );


    Point p1, p2, p3;
    bg::assign_values(p1, lon1, LatitudePolicy::apply(lat1));
    bg::assign_values(p2, lon2, LatitudePolicy::apply(lat2));
    bg::assign_values(p3, lon3, LatitudePolicy::apply(lat3));


    strategy_type strategy;
    return_type d = strategy.apply(p1, p2, p3);

    BOOST_CHECK_CLOSE(radius * d, expected, tolerance);

    // The strategy should return the same result if we reverse the parameters
    d = strategy.apply(p1, p3, p2);
    BOOST_CHECK_CLOSE(radius * d, expected, tolerance);

    // Test specifying radius explicitly
    strategy_type strategy_radius(radius);
    d = strategy_radius.apply(p1, p2, p3);
    BOOST_CHECK_CLOSE(d, expected, tolerance);


    // Test the "default strategy" registration
    bg::model::referring_segment<Point const> segment(p2, p3);
    d = bg::distance(p1, segment);
    BOOST_CHECK_CLOSE(radius * d, expected, tolerance);
}


template <typename Point>
void test_case_boost_geometry_list_20120625()
{
    // This function tests the bug submitted by Karsten Ahnert
    // on Boost.Geometry list at 2012-06-25, and wherefore he
    // submitted a patch a few days later.

    Point p1, p2;
    bg::model::segment<Point> s1, s2;

    bg::read_wkt("POINT(1 1)", p1);
    bg::read_wkt("POINT(5 1)", p2);
    bg::read_wkt("LINESTRING(0 2,2 2)", s1);
    bg::read_wkt("LINESTRING(2 2,4 2)", s2);

    BOOST_CHECK_CLOSE(boost::geometry::distance(p1, s1), 0.0174586, 0.0001);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p1, s2), 0.0246783, 0.0001);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p2, s1), 0.0551745, 0.0001);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p2, s2), 0.0246783, 0.0001);

    // Check degenerated segments
    bg::model::segment<Point> s3;
    bg::read_wkt("LINESTRING(2 2,2 2)", s3);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p1, s3), 0.0246783, 0.0001);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p2, s3), 0.0551745, 0.0001);

    // Point/Point distance should be identical:
    Point p3;
    bg::read_wkt("POINT(2 2)", p3);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p1, p3), 0.0246783, 0.0001);
    BOOST_CHECK_CLOSE(boost::geometry::distance(p2, p3), 0.0551745, 0.0001);
}


template <typename Point, typename LatitudePolicy>
void test_all()
{
    typename bg::coordinate_type<Point>::type const average_earth_radius = 6372795.0;

    // distance (Paris <-> Amsterdam/Barcelona), 
    // with coordinates rounded as below ~87 km
    // is equal to distance (Paris <-> Barcelona/Amsterdam)
    typename bg::coordinate_type<Point>::type const p_to_ab = 86.798321 * 1000.0;
    test_distance<Point, LatitudePolicy>(2, 48, 4, 52, 2, 41, average_earth_radius, p_to_ab, 0.1);
    test_distance<Point, LatitudePolicy>(2, 48, 2, 41, 4, 52, average_earth_radius, p_to_ab, 0.1);

    test_case_boost_geometry_list_20120625<Point>();
}


int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> >, geographic_policy >();

    // NYI: haversine for mathematical spherical coordinate systems
    // test_all<bg::model::point<double, 2, bg::cs::spherical<bg::degree> >, mathematical_policya >();

#if defined(HAVE_TTMATH)
    typedef ttmath::Big<1,4> tt;
    //test_all<bg::model::point<tt, 2, bg::cs::geographic<bg::degree> >, geographic_policy>();
#endif

    return 0;
}
