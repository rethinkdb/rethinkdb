// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_AREA_HPP
#define BOOST_GEOMETRY_TEST_AREA_HPP


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/read.hpp>


template <typename Geometry>
void test_area(Geometry const& geometry,
            typename bg::default_area_result<Geometry>::type expected_area)
{
    typename bg::default_area_result<Geometry>::type area = bg::area(geometry);

#ifdef GEOMETRY_TEST_DEBUG
    std::ostringstream out;
    out << typeid(typename bg::coordinate_type<Geometry>::type).name()
        << " "
        << typeid(typename bg::default_area_result<Geometry>::type).name()
        << " "
        << "area : " << bg::area(geometry)
        << std::endl;
    std::cout << out.str();
#endif

    BOOST_CHECK_CLOSE(area, expected_area, 0.0001);

    // Test with explicitly defined strategies
    bg::strategy::area::surveyor
        <
            typename bg::point_type<Geometry>::type
        > strategy1;

    area = bg::area(geometry, strategy1);

    bg::strategy::area::surveyor
        <
            typename bg::point_type<Geometry>::type,
            typename bg::coordinate_type<Geometry>::type
        > strategy2;

    area = bg::area(geometry, strategy2);

}

template <typename Geometry>
void test_geometry(std::string const& wkt,
            typename bg::default_area_result<Geometry>::type expected_area)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    test_area(geometry, expected_area);
}

template <typename Geometry>
void test_empty_input(Geometry const& geometry)
{
    try
    {
        bg::area(geometry);
    }
    catch(bg::empty_input_exception const& )
    {
        return;
    }
    BOOST_CHECK_MESSAGE(false, "A empty_input_exception should have been thrown" );
}


#endif
