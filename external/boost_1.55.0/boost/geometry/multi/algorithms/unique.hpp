// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_UNIQUE_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_UNIQUE_HPP


#include <boost/range.hpp>

#include <boost/geometry/algorithms/unique.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace unique
{


template <typename Policy>
struct multi_unique
{
    template <typename MultiGeometry, typename ComparePolicy>
    static inline void apply(MultiGeometry& multi, ComparePolicy const& compare)
    {
        for (typename boost::range_iterator<MultiGeometry>::type
                it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            Policy::apply(*it, compare);
        }
    }
};


}} // namespace detail::unique
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// For points, unique is not applicable and does nothing
// (Note that it is not "spatially unique" but that it removes duplicate coordinates,
//  like std::unique does). Spatially unique is "dissolve" which can (or will be)
//  possible for multi-points as well, removing points at the same location.


template <typename MultiLineString>
struct unique<MultiLineString, multi_linestring_tag>
    : detail::unique::multi_unique<detail::unique::range_unique>
{};


template <typename MultiPolygon>
struct unique<MultiPolygon, multi_polygon_tag>
    : detail::unique::multi_unique<detail::unique::polygon_unique>
{};


} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_UNIQUE_HPP
