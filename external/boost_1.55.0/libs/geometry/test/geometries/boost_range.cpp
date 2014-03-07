// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>


#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_range/adjacent_filtered.hpp>
#include <boost/geometry/geometries/adapted/boost_range/filtered.hpp>
#include <boost/geometry/geometries/adapted/boost_range/reversed.hpp>
#include <boost/geometry/geometries/adapted/boost_range/strided.hpp>
#include <boost/geometry/geometries/adapted/boost_range/sliced.hpp>
#include <boost/geometry/geometries/adapted/boost_range/uniqued.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

#include <sstream>

struct not_two
{
    template <typename P>
    bool operator()(P const& p) const
    {
        return boost::geometry::get<0>(p) != 2.0;
    }
};

struct sum_not_five
{
    template <typename P>
    bool operator()(P const& p1, P const& p2) const
    {
        return boost::geometry::get<0>(p1) + boost::geometry::get<0>(p2) != 5.0;
    }
};


template <typename P>
void test_range_adaptor()
{
    bg::model::linestring<P> ls;
    bg::read_wkt("LINESTRING(1 1,2 2,3 3,4 4)", ls);

    {
        std::ostringstream out;
        out << bg::wkt(ls);
        BOOST_CHECK_EQUAL(out.str(), "LINESTRING(1 1,2 2,3 3,4 4)");
    }

    {
        std::ostringstream out;
        out << bg::wkt(ls | boost::adaptors::reversed);
        BOOST_CHECK_EQUAL(out.str(), "LINESTRING(4 4,3 3,2 2,1 1)");
    }

    {
        std::ostringstream out;
        out << bg::wkt(ls | boost::adaptors::strided(2));
        BOOST_CHECK_EQUAL(out.str(), "LINESTRING(1 1,3 3)");
    }

    {
        std::ostringstream out;
        out << bg::wkt(ls | boost::adaptors::sliced(1,3));
        BOOST_CHECK_EQUAL(out.str(), "LINESTRING(2 2,3 3)");
    }

    {
        std::ostringstream out;
        out << bg::wkt(ls | boost::adaptors::filtered(not_two()));
        BOOST_CHECK_EQUAL(out.str(), "LINESTRING(1 1,3 3,4 4)");
    }

    {
        std::ostringstream out;
        out << bg::wkt(ls | boost::adaptors::adjacent_filtered(sum_not_five()));
        BOOST_CHECK_EQUAL(out.str(), "LINESTRING(1 1,3 3,4 4)");
    }

    {
        bg::model::linestring<P> ls2;
        bg::read_wkt("LINESTRING(1 1,1 1,2 2,3 3,3 3,4 4)", ls2);
        std::ostringstream out;


        // uniqued needs == operator, equals
        //out << bg::wkt(ls | boost::adaptors::uniqued);
        //BOOST_CHECK_EQUAL(out.str(), "LINESTRING(1 1,2 2,3 3,4 4)");
    }
}

template <typename P>
void test_all()
{
    test_range_adaptor<P>();
}



int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();

    return 0;
 }

