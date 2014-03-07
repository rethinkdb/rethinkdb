// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/detail/sections/sectionalize.hpp>
#include <boost/geometry/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/views/detail/range_type.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>



template <int DimensionCount, bool Reverse, typename Geometry>
void test_sectionalize(std::string const caseid, Geometry const& geometry, std::size_t section_count)
{
    typedef typename bg::point_type<Geometry>::type point;
    typedef bg::model::box<point> box;
    typedef bg::sections<box, DimensionCount> sections;

    sections s;
    bg::sectionalize<Reverse>(geometry, s);

    BOOST_CHECK_EQUAL(s.size(), section_count);

    typedef typename bg::closeable_view
        <
            typename bg::detail::range_type<Geometry>::type const,
            bg::closure<Geometry>::value
        >::type cview_type;
    typedef typename bg::reversible_view
        <
            cview_type const,
            Reverse ? bg::iterate_reverse : bg::iterate_forward
        >::type view_type;
    typedef typename boost::range_iterator
        <
            view_type const
        >::type range_iterator;

    BOOST_FOREACH(typename sections::value_type const& sec, s)
    {
        cview_type cview(bg::range_by_section(geometry, sec));
        view_type view(cview);
        range_iterator it1 = boost::begin(view) + sec.begin_index;
        range_iterator it2 = boost::begin(view) + sec.end_index;
        int count = 0;
        for (range_iterator it = it1; it != it2; ++it)
        {
            count++;
        }
        BOOST_CHECK_EQUAL(int(sec.count), count);
    }
}

template <typename Geometry, bool Reverse>
void test_sectionalize(std::string const& caseid, std::string const& wkt,
        std::size_t count1)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    if (bg::closure<Geometry>::value == bg::open)
    {
        geometry.outer().resize(geometry.outer().size() - 1);
    }
    //bg::correct(geometry);
    test_sectionalize<1, Reverse>(caseid + "_d1", geometry, count1);
}

template <typename P>
void test_all()
{
    std::string const first = "polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0, 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))";
    test_sectionalize<bg::model::polygon<P>, false>("first", first, 4);

    test_sectionalize<bg::model::polygon<P, false>, true>("first_reverse",
        first, 4);

    test_sectionalize<bg::model::polygon<P, false, true>, false>("first_open",
        first, 4);

    test_sectionalize<bg::model::polygon<P, true, false>, true>("first_open_reverse",
        first, 4);
}

int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();

    return 0;
}
