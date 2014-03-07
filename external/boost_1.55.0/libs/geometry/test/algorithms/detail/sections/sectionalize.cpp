// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/detail/sections/sectionalize.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>


#include <test_common/test_point.hpp>

#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#  include <boost/geometry/algorithms/buffer.hpp>
#  include <boost/geometry/algorithms/centroid.hpp>
#endif



template <int DimensionCount, typename Geometry>
void test_sectionalize_part()
{
    typedef typename bg::point_type<Geometry>::type point_type;
    typedef bg::model::box<point_type> box_type;

    typedef bg::sections<box_type, DimensionCount> sections_type;
    typedef typename boost::range_value<sections_type>::type section_type;

    typedef bg::detail::sectionalize::sectionalize_part
        <
            Geometry, point_type, sections_type, 1, 10
        > sectionalize_part;

    sections_type sections;
    section_type section;


    Geometry geometry;
    geometry.push_back(bg::make<point_type>(1, 1));

    bg::ring_identifier ring_id;
    int index = 0;
    int ndi = 0;
    sectionalize_part::apply(sections, section, index, ndi, geometry, ring_id);
    // There should not yet be anything generated, because it is only ONE point

    geometry.push_back(bg::make<point_type>(2, 2));
    sectionalize_part::apply(sections, section, index, ndi, geometry, ring_id);

}


template <int DimensionCount, bool Reverse, typename G>
void test_sectionalize(std::string const caseid, G const& g, std::size_t section_count,
        std::string const& index_check, std::string const& dir_check)
{
    typedef typename bg::point_type<G>::type point;
    typedef bg::model::box<point> box;
    typedef bg::sections<box, DimensionCount> sections;

    sections s;
    bg::sectionalize<Reverse>(g, s);

    BOOST_CHECK_EQUAL(s.size(), section_count);

    // Check if sections are consecutive and consistent
    int previous_index = -1;
    BOOST_FOREACH(typename sections::value_type const& sec, s)
    {
        if (sec.begin_index > 0)
        {
            BOOST_CHECK_EQUAL(previous_index, sec.begin_index);
        }
        BOOST_CHECK_EQUAL(int(sec.count), int(sec.end_index - sec.begin_index));
        previous_index = sec.end_index;
    }

    // Output streams for sections, boxes, other
    std::ostringstream out_sections;
    std::ostringstream out_boxes;
    std::ostringstream out_dirs;


    for (typename sections::size_type i = 0; i < s.size(); i++)
    {
        box const& b = s[i].bounding_box;

        if (i > 0)
        {
            out_sections << "|";
            out_dirs << "|";
            out_boxes << "|";
        }

        out_sections << s[i].begin_index << ".." << s[i].end_index;
        out_boxes << bg::get<0,0>(b) << " " << bg::get<0,1>(b)
            << ".." << bg::get<1,0>(b) << " " << bg::get<1,1>(b);
        for (int d = 0; d < DimensionCount; d++)
        {
            out_dirs << (d == 0 ? "" : " ");
            switch(s[i].directions[d])
            {
                case -99: out_dirs << "DUP"; break;
                case -1 : out_dirs << "-"; break;
                case  0 : out_dirs << "."; break;
                case +1 : out_dirs << "+"; break;
            }
        }
    }

    if (! index_check.empty())
    {
        BOOST_CHECK_EQUAL(out_sections.str(), index_check);
    }
    if (! dir_check.empty())
    {
        BOOST_CHECK_EQUAL(out_dirs.str(), dir_check);
    }
    else
    {
        if (out_sections.str().length() < 80)
        {
            std::cout << std::endl << bg::wkt(g) << std::endl;
            std::cout << out_sections.str() << std::endl;
            //std::cout << out_boxes.str() << std::endl;
        }
        std::cout << out_dirs.str() << std::endl;
    }

#if defined(TEST_WITH_SVG)
    {
        std::ostringstream filename;
        filename << "sectionalize_"
            << caseid << ".svg";

        std::ofstream svg(filename.str().c_str());

        typedef typename bg::point_type<G>::type point_type;
        bg::svg_mapper<point_type> mapper(svg, 500, 500);

        mapper.add(g);

        static const bool is_line = bg::geometry_id<G>::type::value == 2;
        mapper.map(g, is_line
            ? "opacity:0.6;stroke:rgb(0,0,255);stroke-width:5"
            : "opacity:0.6;fill:rgb(0,0,255);stroke:rgb(0,0,0);stroke-width:0.5");


        for (typename sections::size_type i = 0; i < s.size(); i++)
        {
            box b = s[i].bounding_box;
            bg::buffer(b, b, 0.01);
            mapper.map(b, s[i].duplicate
                ? "fill-opacity:0.4;stroke-opacity:0.6;fill:rgb(0,128,0);stroke:rgb(0,255,0);stroke-width:2.0"
                : "fill-opacity:0.2;stroke-opacity:0.4;fill:rgb(255,0,0);stroke:rgb(255,0,0);stroke-width:0.5");

            std::ostringstream out;

            for (int d = 0; d < DimensionCount; d++)
            {
                out << (d == 0 ? "[" : " ");
                switch(s[i].directions[d])
                {
                    case -99: out << "DUP"; break;
                    case -1 : out << "-"; break;
                    case  0 : out << "."; break;
                    case +1 : out << "+"; break;
                }
            }
            out << "] " << s[i].begin_index << ".." << s[i].end_index;


            point_type p;
            bg::centroid(b, p);
            mapper.text(p, out.str(), "");
        }
    }
#endif

}

template <typename G, bool Reverse>
void test_sectionalize(std::string const& caseid, std::string const& wkt,
        std::size_t count2, std::string const& s2, std::string const d2,
        std::size_t count1, std::string const& s1, std::string const d1)
{
    G g;
    bg::read_wkt(wkt, g);
    test_sectionalize<2, Reverse>(caseid + "_d2", g, count2, s2, d2);
    test_sectionalize<1, Reverse>(caseid + "_d1", g, count1, s1, d1);
}

template <typename P>
void test_all()
{
    test_sectionalize_part<1, bg::model::linestring<P> >();

    test_sectionalize<bg::model::linestring<P>, false>("ls",
        "LINESTRING(1 1,2 2,3 0,5 0,5 8)",
        4, "0..1|1..2|2..3|3..4", "+ +|+ -|+ .|. +",
        2, "0..3|3..4", "+|.");

    // These strings mean:
    // 0..1|1..2 -> first section: [0, 1] | second section [1, 2], etc
    // + +|+ -   -> X increases, Y increases | X increases, Y decreases
    // +|.       -> (only X considered) X increases | X constant

    test_sectionalize<bg::model::polygon<P>, false>("simplex",
        "POLYGON((0 0,0 7,4 2,2 0,0 0))",
        4, "0..1|1..2|2..3|3..4", ". +|+ -|- -|- .",
        //            .   +   -   -   -> 3 sections
        3, "0..1|1..2|2..4", ".|+|-");

    // CCW polygon - orientation is not (always) relevant for sections,
    // they are just generated in the order they come.
    test_sectionalize<bg::model::polygon<P, false>, false>("simplex_ccw",
        "POLYGON((0 0,2 0,4 2,0 7,0 0))",
        4, "0..1|1..2|2..3|3..4", "+ .|+ +|- +|. -",
        //            .   +   -   -   -> 3 sections
        3, "0..2|2..3|3..4", "+|-|.");

    // Open polygon - closeness IS relevant for sections, the
    // last section which is not explicit here should be included.
    // So results are the same as the pre-previous one.
    test_sectionalize<bg::model::polygon<P, true, false>, false>("simplex_open",
        "POLYGON((0 0,0 7,4 2,2 0))",
        4, "0..1|1..2|2..3|3..4", ". +|+ -|- -|- .",
        //            .   +   -   -   -> 3 sections
        3, "0..1|1..2|2..4", ".|+|-");

    test_sectionalize<bg::model::polygon<P>, false>("first",
        "polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0, 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))",
        8, "0..2|2..3|3..4|4..5|5..6|6..8|8..10|10..11", "+ +|+ -|+ +|- +|+ +|+ -|- -|- +",
        4, "0..4|4..5|5..8|8..11", "+|-|+|-");

    test_sectionalize<bg::model::polygon<P, false>, true>("first_reverse",
        "polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0, 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))",
        8, "0..1|1..3|3..5|5..6|6..7|7..8|8..9|9..11", "+ -|+ +|- +|- -|+ -|- -|- +|- -",
        4, "0..3|3..6|6..7|7..11", "+|-|+|-");

    test_sectionalize<bg::model::polygon<P>, false>("second",
        "POLYGON((3 1,2 2,1 3,2 4,3 5,4 4,5 3,4 2,3 1))",
        4, "0..2|2..4|4..6|6..8", "- +|+ +|+ -|- -",
        //        -   -   -   +   +   +   +   -   - -> 3 sections
        3, "0..2|2..6|6..8", "-|+|-");

    // With holes
    test_sectionalize<bg::model::polygon<P>, false>("with_holes",
        "POLYGON((3 1,2 2,1 3,2 4,3 5,4 4,5 3,4 2,3 1), (3 2,2 2,3 4,3 2))",
        7, "0..2|2..4|4..6|6..8|0..1|1..2|2..3", "- +|+ +|+ -|- -|- .|+ +|. -",
        //        -   -   -   +   +   +   +   -   -          -   +   . -> 6 sections
        6, "0..2|2..6|6..8|0..1|1..2|2..3", "-|+|-|-|+|.");

    // With duplicates
    test_sectionalize<bg::model::linestring<P>, false>("with_dups",
        "LINESTRING(1 1,2 2,3 0,3 0,5 0,5 8)",
        5, "0..1|1..2|2..3|3..4|4..5", "+ +|+ -|DUP DUP|+ .|. +",
        4, "0..2|2..3|3..4|4..5", "+|DUP|+|.");
    // With two subsequent duplicate segments
    test_sectionalize<bg::model::linestring<P>, false>("with_subseq_dups",
        "LINESTRING(1 1,2 2,3 0,3 0,3 0,5 0,5 0,5 0,5 0,5 8)",
        6, "0..1|1..2|2..4|4..5|5..8|8..9", "+ +|+ -|DUP DUP|+ .|DUP DUP|. +",
        5, "0..2|2..4|4..5|5..8|8..9", "+|DUP|+|DUP|.");


    typedef bg::model::box<P> B;
    test_sectionalize<2, false, B>("box2", bg::make<B>(0,0,4,4),
            4, "0..1|1..2|2..3|3..4", ". +|+ .|. -|- .");
    test_sectionalize<1, false, B>("box1", bg::make<B>(0,0,4,4),
            4, "0..1|1..2|2..3|3..4", ".|+|.|-");

    return;
    // Buffer-case
    test_sectionalize<bg::model::polygon<P>, false>("buffer",
    "POLYGON((-1.1713 0.937043,2.8287 5.93704,2.90334 6.02339,2.98433 6.10382,2.98433 6.10382,3.07121 6.17786,3.16346 6.24507,3.16346 6.24507,3.16346 6.24507,3.26056 6.30508,3.36193 6.35752,3.36193 6.35752,3.46701 6.40211,3.57517 6.43858,3.57517 6.43858,3.57517 6.43858,3.57517 6.43858,3.68579 6.46672,3.79822 6.48637,3.79822 6.48637,3.91183 6.49741,4.02595 6.49978,4.02595 6.49978,4.02595 6.49978,4.13991 6.49346,4.25307 6.4785,4.25307 6.4785,4.36476 6.45497,4.47434 6.42302,4.47434 6.42302,4.47434 6.42302,4.47434 6.42302,7.47434 5.42302,6.84189 3.52566,4.39043 4.68765,0.390434 -0.312348,-1.1713 0.937043))",
        8, "0..2|2..3|3..4|4..5|5..6|6..8|8..10|10..11", "+ +|+ -|+ +|- +|+ +|+ -|- -|- +",
        4, "0..4|4..5|5..8|8..11", "+|-|+|-");
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    std::string const polygon_li = "POLYGON((1872000 528000,1872000 192000,1536119 192000,1536000 528000,1200000 528000,1200000 863880,1536000 863880,1872000 863880,1872000 528000))";
    bg::model::polygon<int_point_type> int_poly;
    bg::model::polygon<double_point_type> double_poly;
    bg::read_wkt(polygon_li, int_poly);
    bg::read_wkt(polygon_li, double_poly);

    bg::sections<bg::model::box<int_point_type>, 1> int_sections;
    bg::sections<bg::model::box<double_point_type>, 1> double_sections;

    bg::sectionalize<false>(int_poly, int_sections);
    bg::sectionalize<false>(double_poly, double_sections);
    
    bool equally_sized = int_sections.size() == double_sections.size();
    BOOST_CHECK(equally_sized);
    if (! equally_sized)
    {
        return;
    }

    for (unsigned int i = 0; i < int_sections.size(); i++)
    {
        BOOST_CHECK(int_sections[i].begin_index == double_sections[i].begin_index);
        BOOST_CHECK(int_sections[i].count == double_sections[i].count);
    }

}


int test_main(int, char* [])
{
    test_large_integers();

    //test_all<bg::model::d2::point_xy<float> >();
    test_all<bg::model::d2::point_xy<double> >();

    return 0;
}
