// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_TEST_COVERED_BY_HPP
#define BOOST_GEOMETRY_TEST_COVERED_BY_HPP


#include <geometry_test_common.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/io/wkt/read.hpp>


template <typename Geometry1, typename Geometry2>
void test_geometry(std::string const& wkt1,
        std::string const& wkt2, bool expected)
{
    Geometry1 geometry1;
    Geometry2 geometry2;

    bg::read_wkt(wkt1, geometry1);
    bg::read_wkt(wkt2, geometry2);

    bool detected = bg::covered_by(geometry1, geometry2);

    BOOST_CHECK_MESSAGE(detected == expected,
        "covered_by: " << wkt1
        << " in " << wkt2
        << " -> Expected: " << expected
        << " detected: " << detected);
}

/*

template <typename Point, bool Clockwise, bool Closed>
void test_ordered_ring(std::string const& wkt_point,
        std::string const& wkt_geometry, bool expected)
{
    typedef bg::model::ring<Point, Clockwise, Closed> ring_type;
    ring_type ring;
    Point point;

    bg::read_wkt(wkt_geometry, ring);
    if (! Clockwise)
    {
        std::reverse(boost::begin(ring), boost::end(ring));
    }
    if (! Closed)
    {
        ring.resize(ring.size() - 1);
    }

    bg::read_wkt(wkt_point, point);

    bool detected = bg::covered_by(point, ring);

    BOOST_CHECK_MESSAGE(detected == expected,
        "covered_by: " << wkt_point
        << " in " << wkt_geometry
        << " -> Expected: " << expected
        << " detected: " << detected
        << " clockwise: " << int(Clockwise)
        << " closed: " << int(Closed)
        );

    // other strategy (note that this one cannot detect OnBorder
    // (without modifications)

    bg::strategy::covered_by::franklin<Point> franklin;
    detected = bg::covered_by(point, ring, franklin);
    if (! on_border)
    {
        BOOST_CHECK_MESSAGE(detected == expected,
            "covered_by: " << wkt_point
            << " in " << wkt_geometry
            << " -> Expected: " << expected
            << " detected: " << detected
            << " clockwise: " << int(Clockwise)
            << " closed: " << int(Closed)
            );
    }


    bg::strategy::covered_by::crossings_multiply<Point> cm;
    detected = bg::covered_by(point, ring, cm);
    if (! on_border)
    {
        BOOST_CHECK_MESSAGE(detected == expected,
            "covered_by: " << wkt_point
            << " in " << wkt_geometry
            << " -> Expected: " << expected
            << " detected: " << detected
            << " clockwise: " << int(Clockwise)
            << " closed: " << int(Closed)
            );
    }
}

template <typename Point>
void test_ring(std::string const& wkt_point,
        std::string const& wkt_geometry,
        bool expected)
{
    test_ordered_ring<Point, true, true>(wkt_point, wkt_geometry, expected);
    test_ordered_ring<Point, false, true>(wkt_point, wkt_geometry, expected);
    test_ordered_ring<Point, true, false>(wkt_point, wkt_geometry, expected);
    test_ordered_ring<Point, false, false>(wkt_point, wkt_geometry, expected);
    test_geometry<Point, bg::model::polygon<Point> >(wkt_point, wkt_geometry, expected);
}
*/

#endif
