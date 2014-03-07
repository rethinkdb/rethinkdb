// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_CORRECT_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_CORRECT_HPP


#include <boost/range/metafunctions.hpp>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/multi/algorithms/detail/modify.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename MultiPoint>
struct correct<MultiPoint, multi_point_tag>
    : detail::correct::correct_nop<MultiPoint>
{};


template <typename MultiLineString>
struct correct<MultiLineString, multi_linestring_tag>
    : detail::correct::correct_nop<MultiLineString>
{};


template <typename Geometry>
struct correct<Geometry, multi_polygon_tag>
    : detail::multi_modify
        <
            Geometry,
            detail::correct::correct_polygon
                <
                    typename boost::range_value<Geometry>::type
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_CORRECT_HPP
