// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_TEST_UNIQUE_HPP
#define BOOST_GEOMETRY_TEST_UNIQUE_HPP

// Test-functionality, shared between single and multi tests

#include <geometry_test_common.hpp>
#include <boost/geometry/algorithms/unique.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>


template <typename Geometry>
void test_geometry(std::string const& wkt, std::string const& expected)
{
    Geometry geometry;

    bg::read_wkt(wkt, geometry);
    bg::unique(geometry);

    std::ostringstream out;
    out << bg::wkt(geometry);

    BOOST_CHECK_MESSAGE(out.str() == expected,
        "unique: " << wkt
        << " expected " << expected
        << " got " << out.str());
}


#endif
