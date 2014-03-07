// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

#include <geometry_test_common.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/views/box_view.hpp>
#include <boost/geometry/io/wkt/read.hpp>



template <typename Box, bool Reverse>
void test_geometry(std::string const& wkt, std::string const& expected, 
            bg::order_selector expected_order)
{

    Box box;
    bg::read_wkt(wkt, box);

    typedef bg::box_view<Box, Reverse> view_type;
    view_type range(box);

    {
        std::ostringstream out;
        for (typename boost::range_iterator<view_type>::type it = boost::begin(range);
            it != boost::end(range); ++it)
        {
            out << " " << bg::get<0>(*it) << bg::get<1>(*it);
        }
        BOOST_CHECK_EQUAL(out.str(), expected);
    }

    {
        // Check forward/backward behaviour
        typename boost::range_iterator<view_type>::type it = boost::begin(range);
        it++;
        it--;
        // Not verified further, same as segment
    }

    {
        // Check random access behaviour
        int const n = boost::size(range);
        BOOST_CHECK_EQUAL(n, 5);
    }

    // Check Boost.Range concept
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<view_type>) );

    // Check order
    bg::order_selector order = bg::point_order<view_type>::value;
    BOOST_CHECK_EQUAL(order, expected_order);
}


template <typename P>
void test_all()
{
    test_geometry<bg::model::box<P>, true> ("polygon((1 1,2 2))", " 11 12 22 21 11", bg::clockwise);
    test_geometry<bg::model::box<P>, false>("polygon((1 1,2 2))", " 11 21 22 12 11", bg::counterclockwise);
    test_geometry<bg::model::box<P>, true> ("polygon((3 3,5 5))", " 33 35 55 53 33", bg::clockwise);
}


int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();
    return 0;
}
