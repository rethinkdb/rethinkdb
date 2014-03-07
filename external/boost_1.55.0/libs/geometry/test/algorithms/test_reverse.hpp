// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_REVERSE_HPP
#define BOOST_GEOMETRY_TEST_REVERSE_HPP

// Test-functionality, shared between single and multi tests

#include <geometry_test_common.hpp>
#include <boost/geometry/algorithms/reverse.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>


template <typename Geometry>
void test_geometry(std::string const& wkt, std::string const& expected)
{
    Geometry geometry;

    bg::read_wkt(wkt, geometry);
    bg::reverse(geometry);

    std::ostringstream out;
    out << bg::wkt(geometry);

    BOOST_CHECK_MESSAGE(out.str() == expected,
        "reverse: " << wkt
        << " expected " << expected
        << " got " << out.str());
}


#endif
