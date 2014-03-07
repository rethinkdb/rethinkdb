// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_INTERSECTION_HPP
#define BOOST_GEOMETRY_TEST_INTERSECTION_HPP

#include <fstream>
#include <iomanip>

#include <boost/foreach.hpp>

#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/algorithms/num_points.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>


#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

#include <geometry_test_common.hpp>


template <typename OutputType, typename CalculationType, typename G1, typename G2>
typename bg::default_area_result<G1>::type test_intersection(std::string const& caseid,
        G1 const& g1, G2 const& g2,
        std::size_t expected_count = 0, std::size_t expected_point_count = 0,
        double expected_length_or_area = 0,
        double percentage = 0.0001,
        bool debug = false)
{
    bool const is_line = bg::geometry_id<OutputType>::type::value == 2;

    if (debug)
    {
        std::cout << std::endl << "case " << caseid << std::endl;
    }


    typedef typename bg::coordinate_type<G1>::type coordinate_type;
    typedef typename bg::point_type<G1>::type point_type;

    typedef bg::strategy_intersection
        <
            typename bg::cs_tag<point_type>::type,
            G1,
            G2,
            point_type,
            CalculationType
        > strategy;

    // Check both normal behaviour, and _inserter behaviour
    if (! debug)
    {
        std::vector<OutputType> out;
        bg::intersection(g1, g2, out);
    }
    std::vector<OutputType> clip;
    bg::detail::intersection::intersection_insert<OutputType>(g1, g2, std::back_inserter(clip), strategy());


    typename bg::default_area_result<G1>::type length_or_area = 0;
    std::size_t n = 0;
    for (typename std::vector<OutputType>::iterator it = clip.begin();
            it != clip.end();
            ++it)
    {
        if (expected_point_count > 0)
        {
            n += bg::num_points(*it, true);
        }

        // instead of specialization we check it run-time here
        length_or_area += is_line 
            ? bg::length(*it)
            : bg::area(*it);

        if (debug)
        {
            std::cout << std::setprecision(20) << bg::wkt(*it) << std::endl;
        }
    }


#if ! defined(BOOST_GEOMETRY_NO_BOOST_TEST)
    if (expected_point_count > 0)
    {
        BOOST_CHECK_MESSAGE(n == expected_point_count,
                "intersection: " << caseid
                << " #points expected: " << expected_point_count
                << " detected: " << n
                << " type: " << (type_for_assert_message<G1, G2>())
                );
    }

    if (expected_count > 0)
    {
        BOOST_CHECK_MESSAGE(clip.size() == expected_count,
                "intersection: " << caseid
                << " #outputs expected: " << expected_count
                << " detected: " << clip.size()
                << " type: " << (type_for_assert_message<G1, G2>())
                );
    }

    double const detected_length_or_area = boost::numeric_cast<double>(length_or_area);
    BOOST_CHECK_CLOSE(detected_length_or_area, expected_length_or_area, percentage);
#endif


#if defined(TEST_WITH_SVG)
    {
        bool const ccw =
            bg::point_order<G1>::value == bg::counterclockwise
            || bg::point_order<G2>::value == bg::counterclockwise;
        bool const open =
            bg::closure<G1>::value == bg::open
            || bg::closure<G2>::value == bg::open;

        std::ostringstream filename;
        filename << "intersection_"
            << caseid << "_"
            << string_from_type<coordinate_type>::name()
            << string_from_type<CalculationType>::name()
            << (ccw ? "_ccw" : "")
            << (open ? "_open" : "")
            << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<point_type> mapper(svg, 500, 500);

        mapper.add(g1);
        mapper.add(g2);

        mapper.map(g1, is_line
            ? "opacity:0.6;stroke:rgb(0,255,0);stroke-width:5"
            : "fill-opacity:0.5;fill:rgb(153,204,0);"
                    "stroke:rgb(153,204,0);stroke-width:3");
        mapper.map(g2, "fill-opacity:0.3;fill:rgb(51,51,153);"
                    "stroke:rgb(51,51,153);stroke-width:3");

        for (typename std::vector<OutputType>::const_iterator it = clip.begin();
                it != clip.end(); ++it)
        {
            mapper.map(*it, "fill-opacity:0.2;stroke-opacity:0.4;fill:rgb(255,0,0);"
                        "stroke:rgb(255,0,255);stroke-width:8");
        }
    }
#endif

    if (debug)
    {
        std::cout << "end case " << caseid << std::endl;
    }

    return length_or_area;
}

template <typename OutputType, typename G1, typename G2>
typename bg::default_area_result<G1>::type test_one(std::string const& caseid,
        std::string const& wkt1, std::string const& wkt2,
        std::size_t expected_count = 0, std::size_t expected_point_count = 0,
        double expected_length_or_area = 0,
        double percentage = 0.0001,
        bool debug = false)
{
    G1 g1;
    bg::read_wkt(wkt1, g1);

    G2 g2;
    bg::read_wkt(wkt2, g2);

    // Reverse if necessary
    bg::correct(g1);
    bg::correct(g2);

    return test_intersection<OutputType, void>(caseid, g1, g2,
        expected_count, expected_point_count,
        expected_length_or_area, percentage,
        debug);
}

template <typename OutputType, typename Areal, typename Linear>
void test_one_lp(std::string const& caseid,
        std::string const& wkt_areal, std::string const& wkt_linear,
        std::size_t expected_count = 0, std::size_t expected_point_count = 0,
        double expected_length = 0,
        double percentage = 0.0001,
        bool debug1 = false, bool debug2 = false)
{
    Areal areal;
    bg::read_wkt(wkt_areal, areal);
    bg::correct(areal);

    Linear linear;
    bg::read_wkt(wkt_linear, linear);

    test_intersection<OutputType, void>(caseid, areal, linear,
        expected_count, expected_point_count,
        expected_length, percentage, debug1);

    // A linestring reversed should deliver exactly the same.
    bg::reverse(linear);

    test_intersection<OutputType, void>(caseid + "_rev", areal, linear,
        expected_count, expected_point_count,
        expected_length, percentage, debug2);
}

template <typename Geometry1, typename Geometry2>
void test_point_output(std::string const& wkt1, std::string const& wkt2, unsigned int expected_count)
{
    Geometry1 g1;
    bg::read_wkt(wkt1, g1);
    bg::correct(g1);
        
    Geometry2 g2;
    bg::read_wkt(wkt2, g2);
    bg::correct(g2);

    std::vector<typename bg::point_type<Geometry1>::type> points;
    bg::intersection(g1, g2, points);
    BOOST_CHECK_EQUAL(points.size(), expected_count);
}


#endif
