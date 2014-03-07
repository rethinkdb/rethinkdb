// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_EQUALS_HPP
#define BOOST_GEOMETRY_TEST_EQUALS_HPP


#include <geometry_test_common.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/variant/variant.hpp>


template <typename Geometry1, typename Geometry2>
void check_geometry(Geometry1 const& geometry1,
                    Geometry2 const& geometry2,
                    std::string const& caseid,
                    std::string const& wkt1,
                    std::string const& wkt2,
                    bool expected)
{
    bool detected = bg::equals(geometry1, geometry2);

    BOOST_CHECK_MESSAGE(detected == expected,
        "case: " << caseid
        << " equals: " << wkt1
        << " to " << wkt2
        << " -> Expected: " << expected
        << " detected: " << detected);
}


template <typename Geometry1, typename Geometry2>
void test_geometry(std::string const& caseid,
            std::string const& wkt1,
            std::string const& wkt2, bool expected)
{
    Geometry1 geometry1;
    Geometry2 geometry2;

    bg::read_wkt(wkt1, geometry1);
    bg::read_wkt(wkt2, geometry2);

    check_geometry(geometry1, geometry2, caseid, wkt1, wkt2, expected);
    check_geometry(boost::variant<Geometry1>(geometry1), geometry2, caseid, wkt1, wkt2, expected);
    check_geometry(geometry1, boost::variant<Geometry2>(geometry2), caseid, wkt1, wkt2, expected);
    check_geometry(boost::variant<Geometry1>(geometry1), boost::variant<Geometry2>(geometry2), caseid, wkt1, wkt2, expected);
}

#endif
