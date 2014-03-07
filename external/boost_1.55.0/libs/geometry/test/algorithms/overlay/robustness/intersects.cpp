// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>

#define BOOST_GEOMETRY_REPORT_OVERLAY_ERROR
#define BOOST_GEOMETRY_NO_BOOST_TEST
//#define BOOST_GEOMETRY_TIME_OVERLAY

#include <test_overlay_p_q.hpp>

#include <boost/program_options.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/timer.hpp>


template <typename MultiPolygon>
inline void make_polygon(MultiPolygon& mp, int count_x, int count_y, int index, int width_x)
{
    typedef typename bg::point_type<MultiPolygon>::type point_type;

    for(int j = 0; j < count_x; ++j)
    {
        for(int k = 0; k < count_y; ++k)
        {
            mp.push_back(typename MultiPolygon::value_type());
            mp.back().outer().push_back(point_type(width_x + j * 10 + 1, k * 10 + 1));
            mp.back().outer().push_back(point_type(width_x + j * 10 + width_x, k * 10 + 5 + index));
            mp.back().outer().push_back(point_type(width_x + j * 10 + 5 + index, k * 10 + 7));
            mp.back().outer().push_back(point_type(width_x + j * 10 + 1, k * 10 + 1));
        }
    }
}



template <typename MultiPolygon>
void test_intersects(int count_x, int count_y, int width_x, p_q_settings const& settings)
{
    MultiPolygon mp;

    make_polygon(mp, count_x, count_y, 0, width_x);

    bool const b = bg::intersects(mp);
    if (b)
    {
        std::cout << " YES";
    }

    if(settings.svg)
    {
        typedef typename bg::coordinate_type<MultiPolygon>::type coordinate_type;
        typedef typename bg::point_type<MultiPolygon>::type point_type;
        std::ostringstream filename;
        filename << "intersects_"
            << string_from_type<coordinate_type>::name()
            << ".svg";

        std::ofstream svg(filename.str().c_str());
        bg::svg_mapper<point_type> mapper(svg, 500, 500);
        mapper.add(mp);
        mapper.map(mp, "fill-opacity:0.5;fill:rgb(153,204,0);"
                "stroke:rgb(153,204,0);stroke-width:3");
    }
}


template <typename T, bool Clockwise, bool Closed>
void test_all(int count, int count_x, int count_y, int width_x, p_q_settings const& settings)
{
    boost::timer t;

    typedef bg::model::polygon
        <
            bg::model::d2::point_xy<T>, Clockwise, Closed
        > polygon;
    typedef bg::model::multi_polygon
        <
            polygon
        > multi_polygon;


    for(int i = 0; i < count; i++)
    {
        test_intersects<multi_polygon>(count_x, count_y, width_x, settings);
    }
    std::cout
        << " type: " << string_from_type<T>::name()
        << " time: " << t.elapsed()  << std::endl;
}

int main(int argc, char** argv)
{
    try
    {
        namespace po = boost::program_options;
        po::options_description description("=== intersects ===\nAllowed options");

        int width_x = 7;
        int count = 1;
        int count_x = 10;
        int count_y = 10;
        bool ccw = false;
        bool open = false;
        p_q_settings settings;

        description.add_options()
            ("help", "Help message")
            ("count", po::value<int>(&count)->default_value(1), "Number of tests")
            ("count_x", po::value<int>(&count_x)->default_value(10), "Triangle count in x-direction")
            ("count_y", po::value<int>(&count_y)->default_value(10), "Triangle count in y-direction")
            ("width_x", po::value<int>(&width_x)->default_value(7), "Width of triangle in x-direction")
            ("ccw", po::value<bool>(&ccw)->default_value(false), "Counter clockwise polygons")
            ("open", po::value<bool>(&open)->default_value(false), "Open polygons")
            ("wkt", po::value<bool>(&settings.wkt)->default_value(false), "Create a WKT of the inputs, for all tests")
            ("svg", po::value<bool>(&settings.svg)->default_value(false), "Create a SVG for all tests")
        ;

        po::variables_map varmap;
        po::store(po::parse_command_line(argc, argv, description), varmap);
        po::notify(varmap);

        if (varmap.count("help"))
        {
            std::cout << description << std::endl;
            return 1;
        }

        if (ccw && open)
        {
            test_all<double, false, false>(count, count_x, count_y, width_x, settings);
        }
        else if (ccw)
        {
            test_all<double, false, true>(count, count_x, count_y, width_x, settings);
        }
        else if (open)
        {
            test_all<double, true, false>(count, count_x, count_y, width_x, settings);
        }
        else
        {
            test_all<double, true, true>(count, count_x, count_y, width_x, settings);
        }

#if defined(HAVE_TTMATH)
        // test_all<ttmath_big, true, true>(seed, count, max, svg, level);
#endif
    }
    catch(std::exception const& e)
    {
        std::cout << "Exception " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "Other exception" << std::endl;
    }

    return 0;
}
