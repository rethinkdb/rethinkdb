// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef GEOMETRY_TEST_TEST_GEOMETRIES_CUSTOM_SEGMENT_HPP
#define GEOMETRY_TEST_TEST_GEOMETRIES_CUSTOM_SEGMENT_HPP


#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/segment.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace test
{

struct custom_point_for_segment
{
    double x, y;
};


struct custom_segment
{
    custom_point_for_segment one, two;
};

template <typename P>
struct custom_segment_of
{
    P p1, p2;
};

struct custom_segment_4
{
    double a, b, c, d;
};


} // namespace test


BOOST_GEOMETRY_REGISTER_POINT_2D(test::custom_point_for_segment, double, cs::cartesian, x, y)

BOOST_GEOMETRY_REGISTER_SEGMENT(test::custom_segment, test::custom_point_for_segment, one, two)
BOOST_GEOMETRY_REGISTER_SEGMENT_TEMPLATIZED(test::custom_segment_of, p1, p2)
BOOST_GEOMETRY_REGISTER_SEGMENT_2D_4VALUES(test::custom_segment_4, test::custom_point_for_segment, a, b, c, d)


#endif // GEOMETRY_TEST_TEST_GEOMETRIES_CUSTOM_SEGMENT_HPP
