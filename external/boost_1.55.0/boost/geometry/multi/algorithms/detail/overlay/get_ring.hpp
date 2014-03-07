// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP


#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_ring.hpp>
#include <boost/geometry/multi/core/ring_type.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template<>
struct get_ring<multi_polygon_tag>
{
    template<typename MultiPolygon>
    static inline typename ring_type<MultiPolygon>::type const& apply(
                ring_identifier const& id,
                MultiPolygon const& multi_polygon)
    {
        BOOST_ASSERT
            (
                id.multi_index >= 0
                && id.multi_index < int(boost::size(multi_polygon))
            );
        return get_ring<polygon_tag>::apply(id,
                    multi_polygon[id.multi_index]);
    }
};



}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP
