// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP


#include <boost/assert.hpp>
#include <boost/range.hpp>


#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/algorithms/detail/ring_identifier.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template<typename Tag>
struct get_ring
{};

// A container of rings (multi-ring but that does not exist)
// gets the "void" tag and is dispatched here.
template<>
struct get_ring<void>
{
    template<typename Container>
    static inline typename boost::range_value<Container>::type const&
                apply(ring_identifier const& id, Container const& container)
    {
        return container[id.multi_index];
    }
};




template<>
struct get_ring<ring_tag>
{
    template<typename Ring>
    static inline Ring const& apply(ring_identifier const& , Ring const& ring)
    {
        return ring;
    }
};


template<>
struct get_ring<box_tag>
{
    template<typename Box>
    static inline Box const& apply(ring_identifier const& ,
                    Box const& box)
    {
        return box;
    }
};


template<>
struct get_ring<polygon_tag>
{
    template<typename Polygon>
    static inline typename ring_return_type<Polygon const>::type const apply(
                ring_identifier const& id,
                Polygon const& polygon)
    {
        BOOST_ASSERT
            (
                id.ring_index >= -1
                && id.ring_index < int(boost::size(interior_rings(polygon)))
            );
        return id.ring_index < 0
            ? exterior_ring(polygon)
            : interior_rings(polygon)[id.ring_index];
    }
};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_RING_HPP
