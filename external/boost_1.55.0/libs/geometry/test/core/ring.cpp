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


#include <geometry_test_common.hpp>


// To be tested:
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>

// For geometries:
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>


#include <boost/geometry/io/wkt/read.hpp>




template <typename P>
void test_ring(std::string const& wkt,
    std::size_t expected_main_count,
    std::size_t expected_interior_ring_count,
    std::size_t expected_first_interior_count)
{
    typedef bg::model::polygon<P> the_polygon;
    typedef typename bg::ring_type<the_polygon>::type the_ring;
    typedef typename bg::interior_return_type<the_polygon const>::type the_interior;

    the_polygon poly;
    bg::read_wkt(wkt, poly);

    the_ring ext = bg::exterior_ring(poly);
    the_interior rings = bg::interior_rings(poly);

    BOOST_CHECK_EQUAL(bg::num_interior_rings(poly), expected_interior_ring_count);
    BOOST_CHECK_EQUAL(boost::size(rings), expected_interior_ring_count);
    BOOST_CHECK_EQUAL(boost::size(ext), expected_main_count);
    if (boost::size(rings))
    {
        BOOST_CHECK_EQUAL(boost::size(rings.front()), expected_first_interior_count);
    }
}


template <typename P>
void test_all()
{
    test_ring<P>("POLYGON((0 0,0 3,3 3,3 0,0 0),(1 1,1 2,2 2,2 1,1 1))", 5, 1, 5);
    test_ring<P>("POLYGON((0 0,0 3,3 3,3 0,0 0),(1 1,2 2,2 1,1 1),(1 1,1 2,2 2,1 1))", 5, 2, 4);
    test_ring<P>("POLYGON((0 0,0 3,3 3,3 0,0 0))", 5, 0, 0);
}


int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();
    return 0;
}
