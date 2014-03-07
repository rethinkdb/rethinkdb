// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_CENTROID_HPP
#define BOOST_GEOMETRY_TEST_CENTROID_HPP

// Test-functionality, shared between single and multi tests

#include <geometry_test_common.hpp>

#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <boost/geometry/io/wkt/read.hpp>


template <std::size_t D>
struct check_result
{
    template <typename Point1, typename Point2>
    static void apply(Point1 const& actual, Point2 const& expected)
    {
        check_result<D-1>::apply(actual, expected);
        BOOST_CHECK_CLOSE(bg::get<D-1>(actual), bg::get<D-1>(expected), 0.001);
    }
};

template <>
struct check_result<0>
{
    template <typename Point1, typename Point2>
    static void apply(Point1 const&, Point2 const&)
    {}
};


template <typename CalculationType, typename Geometry, typename Point>
void test_with_other_calculation_type(Geometry const& geometry, Point& c1)
{
    typedef typename bg::point_type<Geometry>::type point_type;
    // Calculate it with user defined strategy
    point_type c2;
    bg::centroid(geometry, c2,
        bg::strategy::centroid::bashein_detmer<point_type, point_type, CalculationType>());

    std::cout << typeid(CalculationType).name() << ": " << std::setprecision(20)
        << bg::get<0>(c2) << " " << bg::get<1>(c2)
        << " -> difference: " << bg::distance(c1, c2)
        << std::endl;
}

template <typename Geometry, typename T>
void test_centroid(std::string const& wkt, T const& d1, T const& d2, T const& d3 = T(), T const& d4 = T(), T const& d5 = T())
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    typedef typename bg::point_type<Geometry>::type point_type;
    point_type c1;
    bg::centroid(geometry, c1);
    check_result<bg::dimension<Geometry>::type::value>::apply(c1, boost::make_tuple(d1, d2, d3, d4, d5));

#ifdef REPORT_RESULTS
    std::cout << "normal: " << std::setprecision(20) << bg::get<0>(c1) << " " << bg::get<1>(c1) << std::endl;

    //test_with_other_calculation_type<long long>(geometry, c1);
    test_with_other_calculation_type<float>(geometry, c1);
    test_with_other_calculation_type<long double>(geometry, c1);
#if defined(HAVE_TTMATH)
    test_with_other_calculation_type<ttmath_big>(geometry, c1);
#endif

#endif
}

template <typename Geometry>
void test_centroid_exception()
{
    Geometry geometry;
    try
    {
        typename bg::point_type<Geometry>::type c;
        bg::centroid(geometry, c);
    }
    catch(bg::centroid_exception const& )
    {
        return;
    }
    BOOST_CHECK_MESSAGE(false, "A centroid_exception should have been thrown" );
}


#endif
