// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_CONVEX_HULL_HPP
#define BOOST_GEOMETRY_TEST_CONVEX_HULL_HPP

#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/convex_hull.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/num_points.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>

#include <boost/geometry/geometries/polygon.hpp>


template <typename Geometry, typename Hull>
void test_convex_hull(Geometry const& geometry, Hull const& hull,
                      std::size_t size_original, std::size_t size_hull,
                      double expected_area, bool reverse)
{

    std::size_t n = bg::num_points(hull);

    BOOST_CHECK_MESSAGE(n == size_hull,
        "convex hull: " << bg::wkt(geometry)
        << " -> " << bg::wkt(hull)
        << " type "
        << (typeid(typename bg::coordinate_type<Hull>::type).name())
        << " -> Expected: " << size_hull
        << " detected: " << n);


    // We omit this check as it is not important for the hull algorithm
    // BOOST_CHECK(bg::num_points(geometry) == size_original);

    typename bg::default_area_result<Geometry>::type ah = bg::area(hull);
    if (reverse)
    {
        ah = -ah;
    }

//std::cout << "Area: " << bg::area(geometry) << std::endl;
//std::cout << bg::wkt(hull) << std::endl;

    BOOST_CHECK_CLOSE(ah, expected_area, 0.001);
}

template <typename Geometry, bool Clockwise>
void test_geometry_order(std::string const& wkt,
                      std::size_t size_original, std::size_t size_hull,
                      double expected_area)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);

    bg::model::polygon
        <
            typename bg::point_type<Geometry>::type,
            Clockwise
        > hull;

    // Test version with output iterator
    bg::detail::convex_hull::convex_hull_insert(geometry, std::back_inserter(hull.outer()));
    test_convex_hull(geometry, hull,
        size_original, size_hull, expected_area, ! Clockwise);

    // Test version with ring as output
    bg::clear(hull);
    bg::convex_hull(geometry, hull.outer());
    test_convex_hull(geometry, hull, size_original, size_hull, expected_area, false);

    // Test version with polygon as output
    bg::clear(hull);
    bg::convex_hull(geometry, hull);
    test_convex_hull(geometry, hull, size_original, size_hull, expected_area, false);

    // Test version with strategy
    bg::clear(hull);
    bg::strategy::convex_hull::graham_andrew
        <
            Geometry,
            typename bg::point_type<Geometry>::type
        > graham;
    bg::convex_hull(geometry, hull.outer(), graham);
    test_convex_hull(geometry, hull, size_original, size_hull, expected_area, false);

    // Test version with output iterator and strategy
    bg::clear(hull);
    bg::detail::convex_hull::convex_hull_insert(geometry, std::back_inserter(hull.outer()), graham);
    test_convex_hull(geometry, hull, size_original, size_hull, expected_area, ! Clockwise);
}

template <typename Geometry>
void test_geometry(std::string const& wkt,
                      std::size_t size_original, std::size_t size_hull,
                      double expected_area)
{
    test_geometry_order<Geometry, true>(wkt, size_original, size_hull, expected_area);
    test_geometry_order<Geometry, false>(wkt, size_original, size_hull, expected_area);
}

template <typename Geometry>
void test_empty_input()
{
    Geometry geometry;
    bg::model::polygon
        <
            typename bg::point_type<Geometry>::type
        > hull;

    bg::convex_hull(geometry, hull);
    BOOST_CHECK_MESSAGE(bg::num_points(hull) == 0, "Output convex hull should be empty" );
}



#endif
