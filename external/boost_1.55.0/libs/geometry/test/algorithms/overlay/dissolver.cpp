// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/detail/overlay/dissolver.hpp>

#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/multi.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>


#include <test_common/test_point.hpp>


//#define TEST_WITH_SVG
#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#  include <boost/geometry/io/svg/write_svg_multi.hpp>
#endif

// Collection might be a multi-geometry, or std::vector<ring>
template <typename GeometryOut, typename Collection, typename T>
void test_dissolve_plusmin(std::string const& caseid, Collection const& input,
            T const& expected_positive_area,
            T const& expected_negative_area)
{
    typedef typename boost::range_value<GeometryOut>::type geometry_type;
    typedef typename bg::point_type<geometry_type>::type point_type;


    GeometryOut output;
    bg::dissolver(input, output);

    T zero = T();
    T positive_area = T();
    T negative_area = T();

    BOOST_FOREACH(geometry_type const& geometry, output)
    {
        T a = bg::area(geometry);
        if (a > zero)
        {
            positive_area += a;
        }
        else
        {
            negative_area += a;
        }
    }

    BOOST_CHECK_CLOSE(positive_area, expected_positive_area, 0.001);
    BOOST_CHECK_CLOSE(negative_area, expected_negative_area, 0.001);


#if defined(TEST_WITH_SVG)
    {
        std::ostringstream filename;
        filename << "dissolve_plusmin_"
            << caseid << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<point_type> mapper(svg, 500, 500);

        typedef typename boost::range_value<Collection>::type value_type;
        BOOST_FOREACH(value_type const& geometry, input)
        {
            mapper.add(geometry);
        }

        BOOST_FOREACH(value_type const& geometry, input)
        {
            mapper.map(geometry,
                "opacity:0.6;fill:rgb(0,255,0);stroke:rgb(0,0,0);stroke-width:0.5");
        }
        BOOST_FOREACH(geometry_type const& geometry, output)
        {
            mapper.map(geometry,
                bg::area(geometry) > 0
                ? "opacity:0.5;fill:none;stroke:rgb(255,0,0);stroke-width:5"
                : "opacity:0.5;fill:none;stroke:rgb(0,0,255);stroke-width:5"
                );
        }
    }
#endif

}

template <typename MultiPolygon, typename T>
void test_geometry(std::string const& caseid, std::string const& wkt,
            T const& expected_positive_area,
            T const& expected_negative_area = T())
{

    MultiPolygon multi_polygon;
    bg::read_wkt(wkt, multi_polygon);

    // Test std::vector<Polygon> (= multi_polygon)
    test_dissolve_plusmin<MultiPolygon>(caseid, multi_polygon,
        expected_positive_area,
        expected_negative_area);

    // Test std::vector<ring>
    {
        typedef typename boost::range_value<MultiPolygon>::type polygon_type;
        typedef typename bg::ring_type<MultiPolygon>::type ring_type;
        std::vector<ring_type> rings;
        BOOST_FOREACH(polygon_type const& polygon, multi_polygon)
        {
            rings.push_back(bg::exterior_ring(polygon));
        }

        test_dissolve_plusmin<MultiPolygon>(caseid + "_rings", rings,
            expected_positive_area,
            expected_negative_area);
    }

    // Test different combinations
#define BOOST_GEOMETRY_TEST_PERMUTATIONS
#ifdef BOOST_GEOMETRY_TEST_PERMUTATIONS

    int n = multi_polygon.size();

    // test them in all orders
    std::vector<int> indices;
    for (int i = 0; i < n; i++)
    {
        indices.push_back(i);
    }
    int permutation = 0;
    do
    {
        std::ostringstream out;
        out << caseid;
        MultiPolygon multi_polygon2;
        for (int i = 0; i < n; i++)
        {
            int index = indices[i];
            out << "_" << index;
            multi_polygon2.push_back(multi_polygon[index]);
        }
        test_dissolve_plusmin<MultiPolygon>(out.str(), multi_polygon2, expected_positive_area,
                expected_negative_area);
    } while (std::next_permutation(indices.begin(), indices.end()));
#endif
}

template <typename Point>
void test_all()
{
    typedef bg::model::polygon<Point> polygon;
    typedef bg::model::multi_polygon<polygon> multi_polygon;

    test_geometry<multi_polygon>("simplex_one",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)))",
        7.5);

    test_geometry<multi_polygon>("simplex_two",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)),((2 2,3 6,6 3,2 2)))",
        14.7);
    test_geometry<multi_polygon>("simplex_three",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)),((2 2,3 6,6 3,2 2)),((3 4,5 6,6 2,3 4)))",
        16.7945);
    test_geometry<multi_polygon>("simplex_four",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)),((2 2,3 6,6 3,2 2)),((3 4,5 6,6 2,3 4)),((5 5,7 7,8 4,5 5)))",
        20.7581);

    // disjoint
    test_geometry<multi_polygon>("simplex_disjoint",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)),((1 6,2 10,5 7,1 6)),((3 4,5 6,6 2,3 4)),((6 5,8 7,9 4,6 5)))",
        24.0);

    // new hole of four
    test_geometry<multi_polygon>("new_hole",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)),((2 2,3 6,6 3,2 2)),((3 4,5 6,6 2,3 4)),((3 1,5 4,8 4,3 1)))",
        19.5206);

    // intersection of positive/negative ring
    test_geometry<multi_polygon>("plus_min_one",
        "MULTIPOLYGON(((0 0,1 4,4 1,0 0)),((2 2,6 3,3 6,2 2)))",
        7.5, -7.2);

    // negative ring within a positive ring
    test_geometry<multi_polygon>("plus_min_one_within",
        "MULTIPOLYGON(((0 0,1 7,7 3,0 0)),((1 2,4 4,2 5,1 2)))",
        23.0, -3.5);

    // from buffer
    test_geometry<multi_polygon>("from_buffer_1",
        "MULTIPOLYGON(((2.4 3.03431,1.71716 3.71716,2.4 4,2.4 3.03431))"
            ",((2.4 1.96569,2.4 1,1.71716 1.28284,2.4 1.96569))"
            ",((2.93431 2.5,2.4 3.03431,2.4 1.96569,2.93431 2.5))"
            ",((3.06569 2.5,3 2.43431,2.93431 2.5,3 2.56569,3.06569 2.5))"
            ",((-0.4 5.4,4.4 5.4,4.4 3.83431,3.06569 2.5,4.4 1.16569,4.4 -0.4,-0.4 -0.4,-0.4 5.4)))"
            ,
        26.0596168239, -0.2854871761);

}

int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();
    return 0;
}


