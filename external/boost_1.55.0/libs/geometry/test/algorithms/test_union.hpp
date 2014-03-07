// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_UNION_HPP
#define BOOST_GEOMETRY_TEST_UNION_HPP

#include <fstream>

#include <geometry_test_common.hpp>

#include <boost/range/algorithm/copy.hpp>

#include <boost/geometry/algorithms/union.hpp>

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



template <typename OutputType, typename G1, typename G2>
void test_union(std::string const& caseid, G1 const& g1, G2 const& g2,
        std::size_t expected_count, std::size_t expected_hole_count,
        int expected_point_count, double expected_area,
        double percentage)
{
    typedef typename bg::coordinate_type<G1>::type coordinate_type;
    std::vector<OutputType> clip;
    bg::union_(g1, g2, clip);

    typename bg::default_area_result<G1>::type area = 0;
    std::size_t n = 0;
    std::size_t holes = 0;
    for (typename std::vector<OutputType>::iterator it = clip.begin();
            it != clip.end(); ++it)
    {
        area += bg::area(*it);
        holes += bg::num_interior_rings(*it);
        n += bg::num_points(*it, true);
    }

    {
        // Test inserter functionality
        // Test if inserter returns output-iterator (using Boost.Range copy)
        std::vector<OutputType> inserted, array_with_one_empty_geometry;
        array_with_one_empty_geometry.push_back(OutputType());
        boost::copy(array_with_one_empty_geometry, bg::detail::union_::union_insert<OutputType>(g1, g2, std::back_inserter(inserted)));

        typename bg::default_area_result<G1>::type area_inserted = 0;
        int index = 0;
        for (typename std::vector<OutputType>::iterator it = inserted.begin();
                it != inserted.end();
                ++it, ++index)
        {
            // Skip the empty polygon created above to avoid the empty_input_exception
            if (bg::num_points(*it) > 0)
            {
                area_inserted += bg::area(*it);
            }
        }
        BOOST_CHECK_EQUAL(boost::size(clip), boost::size(inserted) - 1);
        BOOST_CHECK_CLOSE(area_inserted, expected_area, percentage);
    }



    /***
    std::cout << "case: " << caseid
        << " n: " << n
        << " area: " << area
        << " polygons: " << boost::size(clip)
        << " holes: " << holes
        << std::endl;
    ***/

    if (expected_point_count >= 0)
    {
        BOOST_CHECK_MESSAGE(n == std::size_t(expected_point_count),
                "union: " << caseid
                << " #points expected: " << expected_point_count
                << " detected: " << n
                << " type: " << (type_for_assert_message<G1, G2>())
                );
    }

    BOOST_CHECK_EQUAL(clip.size(), expected_count);
    BOOST_CHECK_EQUAL(holes, expected_hole_count);
    BOOST_CHECK_CLOSE(area, expected_area, percentage);

#if defined(TEST_WITH_SVG)
    {
        bool const ccw =
            bg::point_order<G1>::value == bg::counterclockwise
            || bg::point_order<G2>::value == bg::counterclockwise;
        bool const open =
            bg::closure<G1>::value == bg::open
            || bg::closure<G2>::value == bg::open;

        std::ostringstream filename;
        filename << "union_"
            << caseid << "_"
            << string_from_type<coordinate_type>::name()
            << (ccw ? "_ccw" : "")
            << (open ? "_open" : "")
            << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper
            <
                typename bg::point_type<G2>::type
            > mapper(svg, 500, 500);
        mapper.add(g1);
        mapper.add(g2);

        mapper.map(g1, "fill-opacity:0.5;fill:rgb(153,204,0);"
                "stroke:rgb(153,204,0);stroke-width:3");
        mapper.map(g2, "fill-opacity:0.3;fill:rgb(51,51,153);"
                "stroke:rgb(51,51,153);stroke-width:3");
        //mapper.map(g1, "opacity:0.6;fill:rgb(0,0,255);stroke:rgb(0,0,0);stroke-width:1");
        //mapper.map(g2, "opacity:0.6;fill:rgb(0,255,0);stroke:rgb(0,0,0);stroke-width:1");

        for (typename std::vector<OutputType>::const_iterator it = clip.begin();
                it != clip.end(); ++it)
        {
            mapper.map(*it, "fill-opacity:0.2;stroke-opacity:0.4;fill:rgb(255,0,0);"
                    "stroke:rgb(255,0,255);stroke-width:8");
            //mapper.map(*it, "opacity:0.6;fill:none;stroke:rgb(255,0,0);stroke-width:5");
        }
    }
#endif
}

template <typename OutputType, typename G1, typename G2>
void test_one(std::string const& caseid, std::string const& wkt1, std::string const& wkt2,
        std::size_t expected_count, std::size_t expected_hole_count,
        int expected_point_count, double expected_area,
        double percentage = 0.001)
{
    G1 g1;
    bg::read_wkt(wkt1, g1);

    G2 g2;
    bg::read_wkt(wkt2, g2);

    // Reverse if necessary
    bg::correct(g1);
    bg::correct(g2);

    test_union<OutputType>(caseid, g1, g2,
        expected_count, expected_hole_count, expected_point_count,
        expected_area, percentage);
}



#endif
