// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_EQUALS_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_EQUALS_HPP


#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/geometry_id.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/equals.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename MultiPolygon1, typename MultiPolygon2, bool Reverse>
struct equals
    <
        MultiPolygon1, MultiPolygon2,
        multi_polygon_tag, multi_polygon_tag,
        2,
        Reverse
    >
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


template <typename Polygon, typename MultiPolygon, bool Reverse>
struct equals
    <
        Polygon, MultiPolygon,
        polygon_tag, multi_polygon_tag,
        2,
        Reverse
    >
    : detail::equals::equals_by_collection<detail::equals::area_check>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_EQUALS_HPP

