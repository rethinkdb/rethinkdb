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

#include <boost/geometry/views/closeable_view.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/dsv/write.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian);



// The closeable view should also work on normal std:: containers
void test_non_geometry()
{
    typedef bg::closeable_view
        <
            std::vector<int> const, bg::open
        >::type view_type;

    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    view_type view(v);

    typedef boost::range_iterator<view_type const>::type iterator;
    iterator it = boost::begin(view);
    iterator end = boost::end(view);

    std::ostringstream out;
    for ( ; it != end; ++it)
    {
        out << *it;
    }
    BOOST_CHECK_EQUAL(out.str(), "1231");

    // Check operators =, ++, --, +=, -=
    it = boost::begin(view);
    BOOST_CHECK_EQUAL(*it, 1);
    it += 2;
    BOOST_CHECK_EQUAL(*it, 3);
    it -= 2;
    BOOST_CHECK_EQUAL(*it, 1);
    it++;
    BOOST_CHECK_EQUAL(*it, 2);
    it--;
    BOOST_CHECK_EQUAL(*it, 1);

    // Also check them in the last regions
    it = boost::begin(view) + 3;
    BOOST_CHECK_EQUAL(*it, 1);
    it--;
    BOOST_CHECK_EQUAL(*it, 3);
    it++;
    BOOST_CHECK_EQUAL(*it, 1);
    it -= 2;
    BOOST_CHECK_EQUAL(*it, 2);
    it += 2;
    BOOST_CHECK_EQUAL(*it, 1);

    BOOST_CHECK_EQUAL(boost::size(view), 4u);
}


template <bg::closure_selector Closure, typename Range>
void test_optionally_closing(Range const& range, std::string const& expected)
{
    typedef typename bg::closeable_view<Range const, Closure>::type view_type;
    typedef typename boost::range_iterator<view_type const>::type iterator;

    view_type view(range);

    bool first = true;
    std::ostringstream out;
    iterator end = boost::end(view);
    for (iterator it = boost::begin(view); it != end; ++it, first = false)
    {
        out << (first ? "" : " ") << bg::dsv(*it);
    }
    BOOST_CHECK_EQUAL(out.str(), expected);
}


template <typename Geometry>
void test_geometry(std::string const& wkt,
            std::string const& expected_false,
            std::string const& expected_true)
{
    Geometry geo;
    bg::read_wkt(wkt, geo);

    test_optionally_closing<bg::closed>(geo, expected_false);
    test_optionally_closing<bg::open>(geo, expected_true);
}


template <typename P>
void test_all()
{
    test_geometry<bg::model::ring<P> >(
            "POLYGON((1 1,1 4,4 4,4 1))",
            "(1, 1) (1, 4) (4, 4) (4, 1)",
            "(1, 1) (1, 4) (4, 4) (4, 1) (1, 1)");
}


int test_main(int, char* [])
{
    test_non_geometry();

    test_all<bg::model::d2::point_xy<double> >();
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<boost::tuple<double, double> >();

    return 0;
}
