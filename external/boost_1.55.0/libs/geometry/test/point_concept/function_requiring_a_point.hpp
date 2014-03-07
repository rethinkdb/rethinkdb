// Boost.Geometry (aka GGL, Generic Geometry Library) Point concept unit tests
//
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GEOMETRY_TEST_POINT_CONCEPT_FUNCTION_REQUIRING_A_POINT_HPP
#define GEOMETRY_TEST_POINT_CONCEPT_FUNCTION_REQUIRING_A_POINT_HPP

#include <boost/concept/requires.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>

namespace bg = boost::geometry;

namespace test
{
    template <typename P, typename C>

    inline BOOST_CONCEPT_REQUIRES(
        ((bg::concept::Point<P>))
        ((bg::concept::ConstPoint<C>)),
        (void))
    function_requiring_a_point(P& p1, const C& p2)
    {
        bg::set<0>(p1, bg::get<0>(p2));
    }
}

#endif // GEOMETRY_TEST_POINT_CONCEPT_FUNCTION_REQUIRING_A_POINT_HPP
