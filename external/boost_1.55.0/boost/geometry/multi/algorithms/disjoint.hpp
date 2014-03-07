// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012 Bruno Lalande, Paris, France.
// Copyright (c) 2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DISJOINT_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DISJOINT_HPP


#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/multi/algorithms/covered_by.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Point, typename MultiPolygon>
struct disjoint<Point, MultiPolygon, 2, point_tag, multi_polygon_tag, false>
    : detail::disjoint::reverse_covered_by<Point, MultiPolygon>
{};

} // namespace dispatch


#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DISJOINT_HPP
