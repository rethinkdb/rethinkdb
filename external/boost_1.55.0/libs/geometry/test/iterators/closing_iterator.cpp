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

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

#include <geometry_test_common.hpp>

#include <boost/geometry/iterators/closing_iterator.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>


// The closing iterator should also work on normal std:: containers
void test_non_geometry()
{
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    typedef bg::closing_iterator
        <
            std::vector<int> const
        > closing_iterator;


    closing_iterator it(v);
    closing_iterator end(v, true);

    std::ostringstream out;
    for (; it != end; ++it)
    {
        out << *it;
    }
    BOOST_CHECK_EQUAL(out.str(), "1231");
}





template <typename Geometry>
void test_geometry(std::string const& wkt)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    typedef bg::closing_iterator<Geometry const> closing_iterator;


    // 1. Test normal behaviour
    {
        closing_iterator it(geometry);
        closing_iterator end(geometry, true);

        std::ostringstream out;
        for (; it != end; ++it)
        {
            out << " " << bg::get<0>(*it) << bg::get<1>(*it);
        }
        BOOST_CHECK_EQUAL(out.str(), " 11 14 44 41 11");

        // Check compilations, it is (since Oct 2010) random access
        it--;
        --it;
        it += 2;
        it -= 2;
    }

    // 2: check copy behaviour
    {
        typedef typename boost::range_iterator<Geometry const>::type normal_iterator;
        Geometry copy;

        std::copy<closing_iterator>(
            closing_iterator(geometry),
            closing_iterator(geometry, true),
            std::back_inserter(copy));

        std::ostringstream out;
        for (normal_iterator cit = boost::begin(copy); cit != boost::end(copy); ++cit)
        {
                out << " " << bg::get<0>(*cit) << bg::get<1>(*cit);
        }
        BOOST_CHECK_EQUAL(out.str(), " 11 14 44 41 11");
    }
}


template <typename P>
void test_all()
{
    test_non_geometry();
    test_geometry<bg::model::ring<P> >("POLYGON((1 1,1 4,4 4,4 1))");
}


int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();

    return 0;
}
