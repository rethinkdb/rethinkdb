// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.

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
#define BOOST_GEOMETRY_TIME_OVERLAY

#include <test_overlay_p_q.hpp>

#include <boost/program_options.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/timer.hpp>

template <typename Polygon>
inline void make_polygon(Polygon& polygon, int count_x, int count_y, int index, int offset)
{
    typedef typename bg::point_type<Polygon>::type point_type;
    bg::exterior_ring(polygon).push_back(point_type(0, 0));
    bg::exterior_ring(polygon).push_back(point_type(0, count_y * 10));
    bg::exterior_ring(polygon).push_back(point_type(count_x * 10 + 10, count_y * 10));
    bg::exterior_ring(polygon).push_back(point_type(count_x * 10 + 10, 0));
    bg::exterior_ring(polygon).push_back(point_type(0, 0));

    for(int j = 0; j < count_x; ++j)
    {
        for(int k = 0; k < count_y; ++k)
        {
            polygon.inners().push_back(typename Polygon::inner_container_type::value_type());
            polygon.inners().back().push_back(point_type(offset + j * 10 + 1, k * 10 + 1));
            polygon.inners().back().push_back(point_type(offset + j * 10 + 7, k * 10 + 5 + index));
            polygon.inners().back().push_back(point_type(offset + j * 10 + 5 + index, k * 10 + 7));
            polygon.inners().back().push_back(point_type(offset + j * 10 + 1, k * 10 + 1));
        }
    }
    bg::correct(polygon);
}



template <typename Polygon>
void test_star_comb(int count_x, int count_y, int offset, p_q_settings const& settings)
{
    Polygon p, q;

    make_polygon(p, count_x, count_y, 0, 0);
    make_polygon(q, count_x, count_y, 1, offset);

    std::ostringstream out;
    out << "interior_triangles";
    test_overlay_p_q
        <
            Polygon,
            typename bg::coordinate_type<Polygon>::type
        >(out.str(), p, q, settings);
}


template <typename T, bool Clockwise, bool Closed>
void test_all(int count, int count_x, int count_y, int offset, p_q_settings const& settings)
{
    boost::timer t;

    typedef bg::model::polygon
        <
            bg::model::d2::point_xy<T>, Clockwise, Closed
        > polygon;


    for(int i = 0; i < count; i++)
    {
        test_star_comb<polygon>(count_x, count_y, offset, settings);
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
        po::options_description description("=== interior_triangles ===\nAllowed options");

        int offset = 0;
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
            ("offset", po::value<int>(&offset)->default_value(0), "Offset of second triangle in x-direction")
            ("diff", po::value<bool>(&settings.also_difference)->default_value(false), "Include testing on difference")
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
            test_all<double, false, false>(count, count_x, count_y, offset, settings);
        }
        else if (ccw)
        {
            test_all<double, false, true>(count, count_x, count_y, offset, settings);
        }
        else if (open)
        {
            test_all<double, true, false>(count, count_x, count_y, offset, settings);
        }
        else
        {
            test_all<double, true, true>(count, count_x, count_y, offset, settings);
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
