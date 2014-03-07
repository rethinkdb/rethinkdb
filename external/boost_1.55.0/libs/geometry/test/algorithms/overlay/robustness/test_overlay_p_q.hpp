// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test
//
// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_OVERLAY_P_Q_HPP
#define BOOST_GEOMETRY_TEST_OVERLAY_P_Q_HPP

#include <fstream>
#include <iomanip>

//#define BOOST_GEOMETRY_ROBUSTNESS_USE_DIFFERENCE


#include <geometry_test_common.hpp>

// For mixing int/float
#if defined(_MSC_VER)
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#endif


#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/touches.hpp>

struct p_q_settings
{
    bool svg;
    bool also_difference;
    bool wkt;
    double tolerance;

    p_q_settings()
        : svg(false)
        , also_difference(false)
        , wkt(false)
        , tolerance(1.0e-6)
    {}
};

template <typename Geometry>
inline typename bg::default_area_result<Geometry>::type p_q_area(Geometry const& g)
{
    try
    {
        return bg::area(g);
    }
    catch(bg::empty_input_exception const&)
    {
        return 0;
    }
}

template <typename OutputType, typename CalculationType, typename G1, typename G2>
static bool test_overlay_p_q(std::string const& caseid,
            G1 const& p, G2 const& q,
            p_q_settings const& settings)
{
    bool result = true;

    typedef typename bg::coordinate_type<G1>::type coordinate_type;
    typedef typename bg::point_type<G1>::type point_type;

    bg::model::multi_polygon<OutputType> out_i, out_u, out_d, out_d2;

    CalculationType area_p = p_q_area(p);
    CalculationType area_q = p_q_area(q);
    CalculationType area_d1 = 0, area_d2 = 0;

    bg::intersection(p, q, out_i);
    CalculationType area_i = p_q_area(out_i);

    bg::union_(p, q, out_u);
    CalculationType area_u = p_q_area(out_u);

    double sum = (area_p + area_q) - area_u - area_i;

    bool wrong = std::abs(sum) > settings.tolerance;

    if (settings.also_difference)
    {
        bg::difference(p, q, out_d);
        bg::difference(q, p, out_d2);
        area_d1 = p_q_area(out_d);
        area_d2 = p_q_area(out_d2);
        double sum_d1 = (area_u - area_q) - area_d1;
        double sum_d2 = (area_u - area_p) - area_d2;
        bool wrong_d1 = std::abs(sum_d1) > settings.tolerance;
        bool wrong_d2 = std::abs(sum_d2) > settings.tolerance;

        if (wrong_d1 || wrong_d2)
        {
            wrong = true;
        }
    }

    if (true)
    {
        if ((area_i > 0 && bg::touches(p, q))
            || (area_i <= 0 && bg::intersects(p, q) && ! bg::touches(p, q)))
        {
            std::cout << "Wrong 'touch'! " 
                << " Intersection area: " << area_i
                << " Touch gives: " << std::boolalpha << bg::touches(p, q)
                << std::endl;
            wrong = true;
        }
    }

    bool svg = settings.svg;

    if (wrong || settings.wkt)
    {
        if (wrong)
        {
            result = false;
            svg = true;
        }
        bg::unique(out_i);
        bg::unique(out_u);

        std::cout
            << "type: " << string_from_type<CalculationType>::name()
            << " id: " << caseid
            << " area i: " << area_i
            << " area u: " << area_u
            << " area p: " << area_p
            << " area q: " << area_q
            << " sum: " << sum;

        if (settings.also_difference)
        {
            std::cout
                << " area d1: " << area_d1
                << " area d2: " << area_d2;
        }
        std::cout
            << std::endl
            << std::setprecision(9)
            << " p: " << bg::wkt(p) << std::endl
            << " q: " << bg::wkt(q) << std::endl
            << " i: " << bg::wkt(out_i) << std::endl
            << " u: " << bg::wkt(out_u) << std::endl
            ;

    }

    if(svg)
    {
        std::ostringstream filename;
        filename << "overlay_" << caseid << "_"
            << string_from_type<coordinate_type>::name()
            << string_from_type<CalculationType>::name()
            << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<point_type> mapper(svg, 500, 500);

        mapper.add(p);
        mapper.add(q);

        // Input shapes in green/blue
        mapper.map(p, "fill-opacity:0.5;fill:rgb(153,204,0);"
                "stroke:rgb(153,204,0);stroke-width:3");
        mapper.map(q, "fill-opacity:0.3;fill:rgb(51,51,153);"
                "stroke:rgb(51,51,153);stroke-width:3");

        if (settings.also_difference)
        {
            for (BOOST_AUTO(it, out_d.begin()); it != out_d.end(); ++it)
            {
                mapper.map(*it,
                    "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
            }
            for (BOOST_AUTO(it, out_d2.begin()); it != out_d2.end(); ++it)
            {
                mapper.map(*it,
                    "opacity:0.8;fill:none;stroke:rgb(255,0,255);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
            }
        }
        else
        {
            for (BOOST_AUTO(it, out_i.begin()); it != out_i.end(); ++it)
            {
                mapper.map(*it, "fill-opacity:0.1;stroke-opacity:0.4;fill:rgb(255,0,0);"
                        "stroke:rgb(255,0,0);stroke-width:4");
            }
            for (BOOST_AUTO(it, out_u.begin()); it != out_u.end(); ++it)
            {
                mapper.map(*it, "fill-opacity:0.1;stroke-opacity:0.4;fill:rgb(255,0,0);"
                        "stroke:rgb(255,0,255);stroke-width:4");
            }
        }
    }
    return result;
}

#endif // BOOST_GEOMETRY_TEST_OVERLAY_P_Q_HPP
