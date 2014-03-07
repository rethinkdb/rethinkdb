// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP


#include <boost/range.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/overlay/select_rings.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

namespace dispatch
{

    template <typename Multi>
    struct select_rings<multi_polygon_tag, Multi>
    {
        template <typename Geometry, typename Map>
        static inline void apply(Multi const& multi, Geometry const& geometry,
                    ring_identifier id, Map& map, bool midpoint)
        {
            typedef typename boost::range_iterator
                <
                    Multi const
                >::type iterator_type;

            typedef select_rings<polygon_tag, typename boost::range_value<Multi>::type> per_polygon;

            id.multi_index = 0;
            for (iterator_type it = boost::begin(multi); it != boost::end(multi); ++it)
            {
                id.ring_index = -1;
                per_polygon::apply(*it, geometry, id, map, midpoint);
                id.multi_index++;
            }
        }
    };
}


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DETAIL_OVERLAY_SELECT_RINGS_HPP
