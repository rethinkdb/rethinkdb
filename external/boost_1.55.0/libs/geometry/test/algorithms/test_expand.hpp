// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_EXPAND_HPP
#define BOOST_GEOMETRY_TEST_EXPAND_HPP


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/io/dsv/write.hpp>


template <typename Geometry, typename Box>
void test_expand(Box& box,
                  std::string const& wkt,
                  std::string const& expected)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);

    bg::expand(box, geometry);

    std::ostringstream out;
    out << bg::dsv(box, ",", "(", ")", ",", "", "");

    BOOST_CHECK_EQUAL(out.str(), expected);
}

template <typename Geometry, typename Box>
void test_expand_other_strategy(Box& box,
                  std::string const& wkt,
                  std::string const& expected)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);


    bg::expand(box, geometry);

    std::ostringstream out;
    out << bg::dsv(box, ",", "(", ")", ",", "", "");

    BOOST_CHECK_EQUAL(out.str(), expected);
}


#endif
